#include "RuntimeApplication.hpp"
#include "Components.hpp"
#include "window/Window.hpp"

#include <entt/entt.hpp>
#include <functional>

void RuntimeApplication::print_reg() {
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
    auto players = registry.view<Player>();
    for (auto e : players) {
        auto &p = players.get<Player>(e);
        printf("Entity %d: Player(isJumping=%d, jumpForce=%f)\n", entt::to_integral(e), p.isJumping,
               p.jumpForce);
    }
    auto meshes = registry.view<Mesh>();
    for (auto e : meshes) {
        auto &m = meshes.get<Mesh>(e);
        printf("Entity %d: Mesh(modelId=%d)\n", entt::to_integral(e), m.modelId);
    }
    auto materials = registry.view<Material>();
    for (auto e : materials) {
        auto &m = materials.get<Material>(e);
        printf("Entity %d: Material(%f, %f, %f)\n", entt::to_integral(e), m.color.x, m.color.y,
               m.color.z);
    }
}

void RuntimeApplication::start() {
    printf("RuntimeApplication::start()\n");
    for (auto &s : startSystems) {
        s();
    }
}

void RuntimeApplication::update(float dt) {
    // printf("RuntimeApplication::update(dt=%f)\n", dt);
    for (auto &s : updateSystems) {
        s(dt);
    }
}

bool RuntimeApplication::isKeyPressed(int keyCode) const {
    if (_window == nullptr) {
        return false;
    }

    return _window->isKeyPressed(keyCode);
}

bool RuntimeApplication::isKeyJustPressed(int keyCode) const {
    if (_window == nullptr) {
        return false;
    }

    return _window->isKeyJustPressed(keyCode);
}

bool RuntimeApplication::isKeyJustReleased(int keyCode) const {
    if (_window == nullptr) {
        return false;
    }

    return _window->isKeyJustReleased(keyCode);
}

std::pair<float, float> RuntimeApplication::getMousePosition() const {
    if (_window == nullptr) {
        return {0.0f, 0.0f};
    }

    int x = _window->getCursorXPos();
    int y = _window->getCursorYPos();
    return {static_cast<float>(x), static_cast<float>(y)};
}

std::pair<float, float> RuntimeApplication::getMouseDelta() const {
    if (_window == nullptr) {
        return {0.0f, 0.0f};
    }

    auto cursorInfo = _window->getCursorInfo();
    return {static_cast<float>(cursorInfo.cursorMoveInfo.dx),
            static_cast<float>(cursorInfo.cursorMoveInfo.dy)};
}

void RuntimeApplication::registerMesh(int meshId, const std::string &meshPath) {
    meshRegistry[meshId] = meshPath;
    printf("Registered mesh: ID=%d, Path=%s\n", meshId, meshPath.c_str());
}

std::vector<std::pair<int, std::string>> RuntimeApplication::getAllMeshes() const {
    std::vector<std::pair<int, std::string>> meshes;
    for (const auto &[id, path] : meshRegistry) {
        meshes.push_back({id, path});
    }
    return meshes;
}
