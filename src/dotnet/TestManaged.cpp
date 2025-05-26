#include <Windows.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

#include "Components.hpp"
#include "TestManaged.hpp"
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

    // ask nethost for the hostfxr path
    int rc = get_hostfxr_path(buffer, &buf_len, nullptr);
    if (rc != 0) {
        std::cerr << "get_hostfxr_path() failed: 0x" << std::hex << rc << std::endl;
        return false;
    }

    // load hostfxr.dll
    HMODULE lib = ::LoadLibraryW(buffer);
    if (!lib) {
        std::cerr << "LoadLibraryW failed\n";
        return false;
    }

    // reinterpret_cast the FARPROC to each function-pointer type:
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

// Single‐instance App
App &AppSingleton() {
    static App app;
    return app;
}

using RegisterAllFn = void(__cdecl *)(void *);

// signature of your managed callback:
using ManagedPerEntityFn = void (*)(float dt, void **components);

// A getter that, given (registry,entity), returns pointer to that component:
using GetterFn = void *(*)(entt::registry &, entt::entity);

// A storage‐iterator that, given a runtime_view&, calls .iterate(storage<T>())
using IteratorFn = std::function<void(entt::runtime_view &)>;

// ----------------------------------------------------------------------------
// 1) Extend the maps for every component type you have.
//    When you add a new T component, insert here in BOTH maps.
// ----------------------------------------------------------------------------
static const std::unordered_map<std::string, GetterFn> g_getters = {
    {"Transform",
     +[](entt::registry &r, entt::entity e) -> void * { return &r.get<Transform>(e); }},
    {"Velocity", +[](entt::registry &r, entt::entity e) -> void * { return &r.get<Velocity>(e); }},
    // … add your other Component types here …
};

static const std::unordered_map<std::string, IteratorFn> g_storage_iterators = {
    {"Transform",
     [](entt::runtime_view &view) { view.iterate(AppSingleton().registry.storage<Transform>()); }},
    {"Velocity",
     [](entt::runtime_view &view) { view.iterate(AppSingleton().registry.storage<Velocity>()); }},
    // … and here as well …
};

void HostRegisterStartup(void (*sys)()) { AppSingleton().add_startup_system(sys); }

void HostRegisterUpdate(void (*sys)(float)) { AppSingleton().add_update_system(sys); }

uint32_t CreateEntity() { return (uint32_t)AppSingleton().registry.create(); }

void AddTransform(uint32_t e, Transform t) {
    AppSingleton().registry.emplace_or_replace<Transform>(entt::entity{e}, t);
}

void HostRegisterPerEntityUpdate(ManagedPerEntityFn fn, int count, const char *const *names) {
    assert(fn && count > 0);

    // build per‐system vectors of getters + storage‐iterators:
    std::vector<GetterFn> getters;
    getters.reserve(count);
    std::vector<IteratorFn> iters;
    iters.reserve(count);

    for (int i = 0; i < count; ++i) {
        auto &nm = names[i];
        auto git = g_getters.find(nm);
        auto iit = g_storage_iterators.find(nm);
        assert(git != g_getters.end() && "Unknown component for getter");
        assert(iit != g_storage_iterators.end() && "Unknown component for iterator");
        getters.push_back(git->second);
        iters.push_back(iit->second);
    }

    // register one native update‐system lambda
    AppSingleton().add_update_system([=](float dt) {
        // 1) build the dynamic view
        entt::runtime_view view{};
        for (auto &iter : iters) {
            iter(view);
        }

        // 2) per‐entity call
        std::vector<void *> ptrs(count);
        for (auto e : view) {
            for (int i = 0; i < count; ++i) {
                ptrs[i] = getters[i](AppSingleton().registry, e);
            }
            // single P/Invoke for this entity
            fn(dt, ptrs.data());
        }
    });
}

extern "C" {
__declspec(dllexport) __declspec(dllexport) void *__cdecl HostGetProcAddress(char const *name) {
    if (std::strcmp(name, "CreateEntity") == 0) return (void *)&CreateEntity;
    if (std::strcmp(name, "AddTransform") == 0) return (void *)&AddTransform;
    if (std::strcmp(name, "HostRegisterStartup") == 0) return (void *)&HostRegisterStartup;
    if (std::strcmp(name, "HostRegisterUpdate") == 0) return (void *)&HostRegisterUpdate;
    if (std::strcmp(name, "AddTransform") == 0) return (void *)&AddTransform;
    if (std::strcmp(name, "HostRegisterPerEntityUpdate") == 0)
        return (void *)&HostRegisterPerEntityUpdate;
    return nullptr;
}
}

int test_managed() {
    if (!load_hostfxr()) {
        return -1;
    }

    // compute paths
    fs::path root = fs::path(kRootDir);
    fs::path game = root / "build" / "Game";
    fs::path cfg  = game / "Game.runtimeconfig.json";
    fs::path dll  = game / "Game.dll";

    if (!fs::exists(cfg) || !fs::exists(dll)) {
        std::cerr << "Managed files missing\n";
        return -1;
    }

    // get the load_assembly_and_get_function_pointer
    auto load_asm = get_dotnet_load_assembly(cfg.wstring().c_str());
    if (!load_asm) return -1;

    RegisterAllFn register_all_fn = nullptr;

    int rc = load_asm(dll.wstring().c_str(), L"Game.PluginBootstrap, Game", L"RegisterAll",
                      UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&register_all_fn);
    if (rc != 0 || register_all_fn == nullptr) {
        std::cerr << "Failed to bind RegisterAll: 0x" << std::hex << rc << "\n";
        return -1;
    }

    // now call it so managed code will self-register all systems:
    register_all_fn((void *)&HostGetProcAddress);

    App &app = AppSingleton();

    // cs with view, single invoke
    // app.add_update_system([&](float dt) {
    //     // for each entity-with-Transform, call the managed Update on a single component
    //     auto view = app.registry.view<Transform>();
    //     for (auto e : view) {
    //         auto &t = view.get<Transform>(e);
    //         update(dt, &t, 1);
    //     }
    // });

    // cs with storage, multi invoke
    // app.add_update_system([&](float dt) {
    //     // The dense array is guaranteed contiguous *and* stable for this frame
    //     auto &storage = app.registry.storage<Transform>();

    //     Transform **begin = storage.raw();
    //     int count         = static_cast<int>(storage.size());

    //     if (count > 0) update(dt, *begin, count); // single P/Invoke per frame
    // });

    // finally run
    app.run();
    return 0;
}
