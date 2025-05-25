#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <vector>

class App {
  public:
    entt::registry registry;

    // A Startup system is just void()
    using StartupSystem = std::function<void()>;
    // An Update system is void(float dt)
    using UpdateSystem = std::function<void(float)>;

    void add_startup_system(StartupSystem sys) { start_systems.push_back(sys); }
    void add_update_system(UpdateSystem sys) { update_systems.push_back(sys); }

    void run() {
        // 1) run all startup systems once
        for (auto &s : start_systems) s();

        // 2) main loop
        bool running = true;
        while (running) {
            float dt = 1.0f / 60.0f; // just example
            for (auto &s : update_systems) s(dt);

            // ... here you would poll window/input, draw with Vulkan, etc.
        }
    }

  private:
    std::vector<StartupSystem> start_systems;
    std::vector<UpdateSystem> update_systems;
};
