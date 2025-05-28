#pragma once

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export

#include <string>

struct Transform {
    glm::vec3 position;
};

struct Velocity {
    glm::vec3 velocity;
};

struct Model {
    std::string modelPath;
};

struct Player {
    bool isJumping;
    float jumpForce;
};
