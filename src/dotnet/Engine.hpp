#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <iostream>
#include <vector>

#include "Transform.hpp"

class App {
  public:
    entt::registry registry;

    // A Startup system is just void()
    using StartupSystem = std::function<void()>;
    // An Update system is void(float dt)
    using UpdateSystem = std::function<void(float)>;

    void add_startup_system(StartupSystem sys) { startSystems.push_back(sys); }
    void add_update_system(UpdateSystem sys) { updateSystems.push_back(sys); }

    void run() {
        // 1) run all startup systems once
        for (auto &s : startSystems) s();

        // debug
        auto transforms = registry.view<Transform>();
        for (auto e : transforms) {
            auto &t = transforms.get<Transform>(e);
            printf("Entity %d: Transform(%f, %f, %f)\n", entt::to_integral(e), t.x, t.y, t.z);
        }

        // 2) main loop
        // bool running = true;
        while (_updateCount++ < 1000) { // just an example limit
            float dt = 1.0f / 60.0f;    // just example
            for (auto &s : updateSystems) s(dt);

            // ... here you would poll window/input, draw with Vulkan, etc.
        }

        std::cout << "App run complete after " << _updateCount << " updates.\n";

        // debug
        transforms = registry.view<Transform>();
        for (auto e : transforms) {
            auto &t = transforms.get<Transform>(e);
            printf("Entity %d: Transform(%f, %f, %f)\n", entt::to_integral(e), t.x, t.y, t.z);
        }
    }

  private:
    uint32_t _updateCount = 0;

    std::vector<StartupSystem> startSystems;
    std::vector<UpdateSystem> updateSystems;
};
