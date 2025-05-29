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

struct Mesh {
    int32_t modelId;  // 使用ID而不是字符串路径，避免marshalling问题
};

struct Material {
    glm::vec3 color;
};