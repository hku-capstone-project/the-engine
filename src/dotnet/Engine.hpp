#pragma once

#include <chrono> // added for timing
#include <entt/entt.hpp>
#include <functional>
#include <iostream>
#include <vector>

#include "Components.hpp"

class App {
  public:
    entt::registry registry;

    // A Startup system is just void()
    using StartupSystem = std::function<void()>;
    // An Update system is void(float dt)
    using UpdateSystem = std::function<void(float)>;

    void add_startup_system(StartupSystem sys) { startSystems.push_back(sys); }
    void add_update_system(UpdateSystem sys) { updateSystems.push_back(sys); }

    void print_reg() {
        auto transforms = registry.view<Transform>();
        for (auto e : transforms) {
            auto &t = transforms.get<Transform>(e);
            printf("Entity %d: Transform(%f, %f, %f)\n", entt::to_integral(e), t.position.x,
                   t.position.y, t.position.z);
        }
        auto velocities = registry.view<Velocity>();
        for (auto e : velocities) {
            auto &v = velocities.get<Velocity>(e);
            printf("Entity %d: Velocity(%f, %f, %f)\n", entt::to_integral(e), v.velocity.x,
                   v.velocity.y, v.velocity.z);
        }
    }

    void run() {
        for (auto &s : startSystems) {
            s();
        }

        print_reg();

        // start measure current time
        auto start = std::chrono::high_resolution_clock::now();

        while (_updateCount++ < 1000) { // just an example limit
            float dt = 1.0f / 60.0f;    // just example
            for (auto &s : updateSystems) s(dt);

            // ... here you would poll window/input, draw with Vulkan, etc.
        }

        // stop measure current time and print elapsed time
        auto end = std::chrono::high_resolution_clock::now();
        double elapsedSeconds =
            std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();

        std::cout << "App run complete after " << _updateCount << " updates in " << elapsedSeconds
                  << " seconds.\n";

        print_reg();
    }

  private:
    uint32_t _updateCount = 0;

    std::vector<StartupSystem> startSystems;
    std::vector<UpdateSystem> updateSystems;
};
