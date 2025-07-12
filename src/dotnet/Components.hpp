#pragma once

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export

#include <string>

struct Transform {
    glm::vec3 position;
    glm::vec3 scale;
};

struct Velocity {
    glm::vec3 velocity;
};

// struct Model {
//     std::string modelPath;
// };

struct Player {
    bool isJumping;
    float jumpForce;
};

struct Mesh {
    int32_t modelId;  // 使用ID而不是字符串路径，避免marshalling问题
};

struct Material {
    glm::vec3 color;
    float metallic = 0.0;
    float roughness = 0.5;
    float occlusion = 1.0;
    glm::vec3 emissive = glm::vec3(0.0f);
};

class Components{
    public:
    Transform transform;
    Velocity velocity;
    Player player;
    Mesh mesh;
    Material material;
    // 可以添加更多组件
};