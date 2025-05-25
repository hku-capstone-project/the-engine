#include <Windows.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

#include "MyStruct.hpp"
#include "TestManaged.hpp"
#include "config/RootDir.h"

#include "coreclr_delegates.h"
#include "hostfxr.h"
#include "nethost.h"

using string_t = std::basic_string<char_t>;
namespace fs   = std::filesystem;

// hostfxr entry-points
static hostfxr_initialize_for_runtime_config_fn init_fptr = nullptr;
static hostfxr_get_runtime_delegate_fn get_delegate_fptr  = nullptr;
static hostfxr_close_fn close_fptr                        = nullptr;

//----------------------------------------------------------------------
// load hostfxr.dll and bind our three entrypoints
//----------------------------------------------------------------------
void *load_library(const char_t *path) {
    HMODULE h = ::LoadLibraryW(path);
    assert(h && "Failed to load hostfxr.dll");
    return (void *)h;
}

// 1) Change get_export to return FARPROC
static FARPROC get_export(HMODULE h, const char *name) {
    FARPROC p = ::GetProcAddress(h, name);
    assert(p && "Failed to get hostfxr export");
    return p;
}

bool load_hostfxr() {
    char_t buffer[MAX_PATH];
    size_t buf_len = sizeof(buffer) / sizeof(char_t);

    // 1) ask nethost for the hostfxr path
    int rc = get_hostfxr_path(buffer, &buf_len, nullptr);
    if (rc != 0) {
        std::cerr << "get_hostfxr_path() failed: 0x" << std::hex << rc << std::endl;
        return false;
    }

    // 2) load hostfxr.dll
    HMODULE lib = ::LoadLibraryW(buffer);
    if (!lib) {
        std::cerr << "LoadLibraryW failed\n";
        return false;
    }

    // 2) reinterpret_cast the FARPROC to each function-pointer type:
    init_fptr = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
        get_export(lib, "hostfxr_initialize_for_runtime_config"));
    get_delegate_fptr = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
        get_export(lib, "hostfxr_get_runtime_delegate"));
    close_fptr = reinterpret_cast<hostfxr_close_fn>(get_export(lib, "hostfxr_close"));

    return init_fptr && get_delegate_fptr && close_fptr;
}

//----------------------------------------------------------------------
// initialize CoreCLR and grab the load_assembly_and_get_function_pointer
//----------------------------------------------------------------------
load_assembly_and_get_function_pointer_fn get_dotnet_load_assembly(const char_t *config_path) {
    hostfxr_handle cxt = nullptr;
    int rc             = init_fptr(config_path, nullptr, &cxt);
    if (rc != 0 || cxt == nullptr) {
        std::cerr << "hostfxr_initialize_for_runtime_config() failed: 0x" << std::hex << rc
                  << std::endl;
        return nullptr;
    }

    void *fn = nullptr;
    rc       = get_delegate_fptr(cxt, hdt_load_assembly_and_get_function_pointer, &fn);
    if (rc != 0 || fn == nullptr) {
        std::cerr << "hostfxr_get_runtime_delegate() failed: 0x" << std::hex << rc << std::endl;
        close_fptr(cxt);
        return nullptr;
    }

    close_fptr(cxt);
    return (load_assembly_and_get_function_pointer_fn)fn;
}

int test_managed() {
    if (!load_hostfxr()) return -1;

    // compute paths
    fs::path root    = fs::path(kRootDir);
    fs::path managed = root / "build" / "Managed";
    fs::path cfg     = managed / "HelloManaged.runtimeconfig.json";
    fs::path dll     = managed / "HelloManaged.dll";

    if (!fs::exists(cfg) || !fs::exists(dll)) {
        std::cerr << "Managed files missing\n";
        return -1;
    }

    // get the load_assembly_and_get_function_pointer
    auto load_fn = get_dotnet_load_assembly(cfg.wstring().c_str());
    if (!load_fn) return -1;

    // Prepare to bind Entrance.MutateStruct
    using mutate_fn   = void(__cdecl *)(void *);
    mutate_fn mutator = nullptr;

    string_t type_name   = L"HelloManaged.Entrance, HelloManaged";
    const char_t *method = L"MutateStruct";

    int rc = load_fn(dll.wstring().c_str(), type_name.c_str(), method,
                     UNMANAGEDCALLERSONLY_METHOD, // must match UnmanagedCallersOnly
                     nullptr, (void **)&mutator);

    if (rc != 0 || mutator == nullptr) {
        std::cerr << "Failed to bind MutateStruct: 0x" << std::hex << rc << "\n";
        return -1;
    }

    // NOW: allocate + initialize a native struct
    MyStruct s;
    s.x = 1;
    s.y = 2;
    std::cout << "[C++] Before call: (x,y)=(" << s.x << "," << s.y << ")\n";

    // call into C# and pass &s
    mutator(&s);

    // after return, the C# code has changed s.x and s.y
    std::cout << "[C++] After call:  (x,y)=(" << s.x << "," << s.y << ")\n";

    return 0;
}
