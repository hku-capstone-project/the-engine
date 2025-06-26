#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <vector>

class Window;

class RuntimeApplication {
  public:
    entt::registry registry;

    // A Startup system is just void()
    using StartupSystem = std::function<void()>;
    // An Update system is void(float dt)
    using UpdateSystem = std::function<void(float)>;

    inline void add_startup_system(StartupSystem sys) { startSystems.push_back(sys); }
    inline void add_update_system(UpdateSystem sys) { updateSystems.push_back(sys); }

    void print_reg();

    // Should only be called once
    void start();

    // Should be called every frame, timing should be namaged by the main application
    void update(float dt);

    // Set the window reference for keyboard input access
    void setWindow(Window* window) { _window = window; }

    // Get keyboard input state
    bool isKeyPressed(int keyCode) const;
    bool isKeyJustPressed(int keyCode) const;
    bool isKeyJustReleased(int keyCode) const;

  private:
    uint32_t _updateCount = 0;
    Window* _window = nullptr;

    std::vector<StartupSystem> startSystems;
    std::vector<UpdateSystem> updateSystems;
};
