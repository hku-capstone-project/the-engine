#include "RuntimeBridge.hpp"
#include "Components.hpp"
#include "RuntimeApplication.hpp"
#include "config/RootDir.h"
#include "coreclr_delegates.h"
#include "hostfxr.h"
#include "nethost.h"
#include "utils/logger/Logger.hpp"

#include <Windows.h>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

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
RuntimeApplication &RuntimeBridge::getRuntimeApplication() {
    static RuntimeApplication app;
    return app;
}

using RegisterAllFn = void(__cdecl *)(void *);

// signature of your managed callback:
using ManagedPerEntityFn = void (*)(float dt, void **components);

// A getter that, given (registry,entity), returns pointer to that component:
using GetterFn = void *(*)(entt::registry &, entt::entity);

// A storage‐iterator that, given a runtime_view&, calls .iterate(storage<T>())
using IteratorFn = std::function<void(entt::runtime_view &)>;

// when you add a new T component, insert here in BOTH maps.
static const std::unordered_map<std::string, GetterFn> g_getters = {
    {"Transform",
     +[](entt::registry &r, entt::entity e) -> void * { return &r.get<Transform>(e); }},
    {"iCamera",
     +[](entt::registry &r, entt::entity e) -> void * { return &r.get<iCamera>(e); }},
    {"Velocity", +[](entt::registry &r, entt::entity e) -> void * { return &r.get<Velocity>(e); }},
    {"Player", +[](entt::registry &r, entt::entity e) -> void * { return &r.get<Player>(e); }},
    {"Mesh", +[](entt::registry &r, entt::entity e) -> void * { return &r.get<Mesh>(e); }},
    {"Material", +[](entt::registry &r, entt::entity e) -> void * { return &r.get<Material>(e); }},
    {"GameStats", +[](entt::registry &r, entt::entity e) -> void * { return &r.get<GameStats>(e); }},
};

static const std::unordered_map<std::string, IteratorFn> g_storage_iterators = {
    {"Transform",
     [](entt::runtime_view &view) {
         view.iterate(RuntimeBridge::getRuntimeApplication().registry.storage<Transform>());
     }},
     {"iCamera",
     [](entt::runtime_view &view) {
         view.iterate(RuntimeBridge::getRuntimeApplication().registry.storage<iCamera>());
     }},
    {"Velocity",
     [](entt::runtime_view &view) {
         view.iterate(RuntimeBridge::getRuntimeApplication().registry.storage<Velocity>());
     }},
    {"Player",
     [](entt::runtime_view &view) {
         view.iterate(RuntimeBridge::getRuntimeApplication().registry.storage<Player>());
     }},
    {"Mesh",
     [](entt::runtime_view &view) {
         view.iterate(RuntimeBridge::getRuntimeApplication().registry.storage<Mesh>());
     }},
    {"Material",
     [](entt::runtime_view &view) {
         view.iterate(RuntimeBridge::getRuntimeApplication().registry.storage<Material>());
     }},
    {"GameStats",
     [](entt::runtime_view &view) {
         view.iterate(RuntimeBridge::getRuntimeApplication().registry.storage<GameStats>());
     }},
};

void HostRegisterStartup(void (*sys)()) {
    RuntimeBridge::getRuntimeApplication().add_startup_system(sys);
}

void HostRegisterUpdate(void (*sys)(float)) {
    RuntimeBridge::getRuntimeApplication().add_update_system(sys);
}

uint32_t CreateEntity() {
    return (uint32_t)RuntimeBridge::getRuntimeApplication().registry.create();
}

void AddTransform(uint32_t e, Transform t) {
    RuntimeBridge::getRuntimeApplication().registry.emplace_or_replace<Transform>(entt::entity{e},
                                                                                  t);
}
void AddCamera(uint32_t e, iCamera c) {
    RuntimeBridge::getRuntimeApplication().registry.emplace_or_replace<iCamera>(entt::entity{e},
                                                                                  c);
}
void AddVelocity(uint32_t e, Velocity v) {
    RuntimeBridge::getRuntimeApplication().registry.emplace_or_replace<Velocity>(entt::entity{e},
                                                                                 v);
}
void AddPlayer(uint32_t e, Player p) {
    RuntimeBridge::getRuntimeApplication().registry.emplace_or_replace<Player>(entt::entity{e}, p);
}
void AddMesh(uint32_t e, Mesh m) {
    RuntimeBridge::getRuntimeApplication().registry.emplace_or_replace<Mesh>(entt::entity{e}, m);
}
void AddMaterial(uint32_t e, Material m) {
    RuntimeBridge::getRuntimeApplication().registry.emplace_or_replace<Material>(entt::entity{e},
                                                                                 m);
}
void AddGameStats(uint32_t e, GameStats g) {
    RuntimeBridge::getRuntimeApplication().registry.emplace_or_replace<GameStats>(entt::entity{e},
                                                                                 g);
}

void HostRemoveComponentTransform(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.remove<Transform>(entt::entity{entityId});
}

void HostRemoveComponentCamera(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.remove<iCamera>(entt::entity{entityId});
}

void HostRemoveComponentVelocity(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.remove<Velocity>(entt::entity{entityId});
}
void HostRemoveComponentPlayer(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.remove<Player>(entt::entity{entityId});
}
void HostRemoveComponentMesh(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.remove<Mesh>(entt::entity{entityId});
}
void HostRemoveComponentMaterial(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.remove<Material>(entt::entity{entityId});
}
void HostRemoveComponentGameStats(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.remove<GameStats>(entt::entity{entityId});
}
void HostDestroyEntity(uint32_t entityId) {
    RuntimeBridge::getRuntimeApplication().registry.destroy(entt::entity{entityId});
}

// Add keyboard input function
bool IsKeyPressed(int keyCode) {
    return RuntimeBridge::getRuntimeApplication().isKeyPressed(keyCode);
}

bool IsKeyJustPressed(int keyCode) {
    return RuntimeBridge::getRuntimeApplication().isKeyJustPressed(keyCode);
}

bool IsKeyJustReleased(int keyCode) {
    return RuntimeBridge::getRuntimeApplication().isKeyJustReleased(keyCode);
}

void RegisterMesh(int meshId, const char* meshPath) {
    RuntimeBridge::getRuntimeApplication().registerMesh(meshId, std::string(meshPath));
}

// Add mouse input functions
void GetMousePosition(float* x, float* y) {
    auto mousePos = RuntimeBridge::getRuntimeApplication().getMousePosition();
    *x = mousePos.first;
    *y = mousePos.second;
}

void GetMouseDelta(float* dx, float* dy) {
    auto mouseDelta = RuntimeBridge::getRuntimeApplication().getMouseDelta();
    *dx = mouseDelta.first;
    *dy = mouseDelta.second;
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
    RuntimeBridge::getRuntimeApplication().add_update_system([=](float dt) {
        // 1) build the dynamic view
        entt::runtime_view view{};
        for (auto &iter : iters) {
            iter(view);
        }

        // 2) per‐entity call
        std::vector<void *> ptrs(count);
        for (auto e : view) {
            for (int i = 0; i < count; ++i) {
                ptrs[i] = getters[i](RuntimeBridge::getRuntimeApplication().registry, e);
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
    if (std::strcmp(name, "AddCamera") == 0) return (void *)&AddCamera;
    if (std::strcmp(name, "AddVelocity") == 0) return (void *)&AddVelocity;
    if (std::strcmp(name, "AddPlayer") == 0) return (void *)&AddPlayer;
    if (std::strcmp(name, "AddMesh") == 0) return (void *)&AddMesh;
    if (std::strcmp(name, "AddMaterial") == 0) return (void *)&AddMaterial;
    if (std::strcmp(name, "AddGameStats") == 0) return (void *)&AddGameStats;
    if (std::strcmp(name, "HostRemoveComponentTransform") == 0)
        return (void *)&HostRemoveComponentTransform;
    if (std::strcmp(name, "HostRemoveComponentCamera") == 0)
        return (void *)&HostRemoveComponentCamera;
    if (std::strcmp(name, "HostRemoveComponentVelocity") == 0)
        return (void *)&HostRemoveComponentVelocity;
    if (std::strcmp(name, "HostRemoveComponentPlayer") == 0)
        return (void *)&HostRemoveComponentPlayer;
    if (std::strcmp(name, "HostRemoveComponentMesh") == 0) return (void *)&HostRemoveComponentMesh;
    if (std::strcmp(name, "HostRemoveComponentMaterial") == 0)
        return (void *)&HostRemoveComponentMaterial;
    if (std::strcmp(name, "HostRemoveComponentGameStats") == 0)
        return (void *)&HostRemoveComponentGameStats;
    if (std::strcmp(name, "HostDestroyEntity") == 0) return (void *)&HostDestroyEntity;
    if (std::strcmp(name, "HostRegisterPerEntityUpdate") == 0)
        return (void *)&HostRegisterPerEntityUpdate;
    if (std::strcmp(name, "IsKeyPressed") == 0) return (void *)&IsKeyPressed;
    if (std::strcmp(name, "IsKeyJustPressed") == 0) return (void *)&IsKeyJustPressed;
    if (std::strcmp(name, "IsKeyJustReleased") == 0) return (void *)&IsKeyJustReleased;
    if (std::strcmp(name, "RegisterMesh") == 0) return (void *)&RegisterMesh;
    if (std::strcmp(name, "GetMousePosition") == 0) return (void *)&GetMousePosition;
    if (std::strcmp(name, "GetMouseDelta") == 0) return (void *)&GetMouseDelta;
    return nullptr;
}
}

void RuntimeBridge::bootstrap(Logger *logger) {
    if (!load_hostfxr()) {
        logger->error("Failed to load hostfxr");
        return;
    }

    // compute paths
    fs::path root = fs::path(kRootDir);
    fs::path game = root / "build" / "Game";
    fs::path cfg  = game / "Game.runtimeconfig.json";
    fs::path dll  = game / "Game.dll";

    if (!fs::exists(cfg) || !fs::exists(dll)) {
        logger->error("Managed files missing");
        return;
    }

    // get the load_assembly_and_get_function_pointer
    auto load_asm = get_dotnet_load_assembly(cfg.wstring().c_str());
    if (!load_asm) {
        logger->error("Failed to get dotnet load assembly");
        return;
    }

    RegisterAllFn register_all_fn = nullptr;

    int rc = load_asm(dll.wstring().c_str(), L"Game.PluginBootstrap, Game", L"RegisterAll",
                      UNMANAGEDCALLERSONLY_METHOD, nullptr, (void **)&register_all_fn);
    if (rc != 0 || register_all_fn == nullptr) {
        logger->error("Failed to bind RegisterAll: 0x" + std::to_string(rc));
        return;
    }

    // now call it so managed code will self-register all systems:
    register_all_fn((void *)&HostGetProcAddress);
}
