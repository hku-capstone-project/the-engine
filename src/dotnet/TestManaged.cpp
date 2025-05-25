#include <Windows.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

#include "TestManaged.hpp"
#include "Transform.hpp"
#include "config/RootDir.h"

#include "Engine.hpp"

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

// managed function pointers
using InitFn   = void(__cdecl *)(void   */*createEntityPtr*/, void   */*addTransformPtr*/);
using UpdateFn = void(__cdecl *)(float /*dt*/, Transform * /*ptr*/, int /*count*/);

// Single‐instance App
App &AppSingleton() {
    static App app;
    return app;
}

// forward C ABI for CreateEntity/AddTransform
extern "C" {
__declspec(dllexport) uint32_t __cdecl CreateEntity() {
    return (uint32_t)AppSingleton().registry.create();
}
__declspec(dllexport) void __cdecl AddTransform(uint32_t e, Transform t) {
    AppSingleton().registry.emplace<Transform>(entt::entity{e}, t);
}
}

static void UpdateTransformsCpp(float dt, Transform *ptr, int count) {
    for (int i = 0; i < count; ++i) {
        auto &t = ptr[i];
        t.z     = std::sin((t.x + t.y) + dt * 3.0f) * 0.5f;
    }
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
    auto load_asm = get_dotnet_load_assembly(cfg.wstring().c_str());
    if (!load_asm) return -1;

    // using mutate_fn   = void(__cdecl *)(void *);
    // mutate_fn mutator = nullptr;
    InitFn init     = nullptr;
    UpdateFn update = nullptr;

    int rc = load_asm(dll.wstring().c_str(), L"HelloManaged.GameScript, HelloManaged", L"Init",
                      UNMANAGEDCALLERSONLY_METHOD, // must match UnmanagedCallersOnly
                      nullptr, (void **)&init);
    if (rc != 0 || init == nullptr) {
        std::cerr << "Failed to bind Init: 0x" << std::hex << rc << "\n";
        return -1;
    }

    rc = load_asm(dll.wstring().c_str(), L"HelloManaged.GameScript, HelloManaged", L"Update",
                  UNMANAGEDCALLERSONLY_METHOD, // must match UnmanagedCallersOnly
                  nullptr, (void **)&update);
    if (rc != 0 || update == nullptr) {
        std::cerr << "Failed to bind Update: 0x" << std::hex << rc << "\n";
        return -1;
    }

    App &app = AppSingleton();
    app.add_startup_system([&]() {
        // tell C# how to spawn entities & add transforms
        init((void *)&CreateEntity, (void *)&AddTransform);
    });

    // cs with view, single invoke
    // 2.95s
    // app.add_update_system([&](float dt) {
    //     // for each entity-with-Transform, call the managed Update on a single component
    //     auto view = app.registry.view<Transform>();
    //     for (auto e : view) {
    //         auto &t = view.get<Transform>(e);
    //         update(dt, &t, 1);
    //     }
    // });

    // C++ with view, single invoke
    // 1.79s
    // app.add_update_system([&](float dt) {
    //     auto view = app.registry.view<Transform>();
    //     for (auto e : view) {
    //         auto &t = view.get<Transform>(e);
    //         UpdateTransformsCpp(dt, &t, 1);
    //     }
    // });

    // cs with storage, multi invoke
    // 1.80s
    app.add_update_system([&](float dt) {
        // The dense array is guaranteed contiguous *and* stable for this frame
        auto &storage = app.registry.storage<Transform>();

        Transform **begin = storage.raw();
        int count         = static_cast<int>(storage.size());

        if (count > 0) update(dt, *begin, count); // single P/Invoke per frame
    });

    // C++ with storage, multi invoke
    // 1.24s
    // app.add_update_system([&](float dt) {
    //     auto &storage    = app.registry.storage<Transform>();
    //     Transform **data = storage.raw();
    //     int count        = static_cast<int>(storage.size());
    //     if (count > 0) UpdateTransformsCpp(dt, *data, count);
    // });

    // finally run
    app.run();
    return 0;
}
