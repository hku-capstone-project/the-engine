#pragma once

#include "utils/incl/GlmIncl.hpp" // IWYU pragma: export

#include <string>

struct Transform {
    glm::vec3 position;
    glm::vec3 rotation; // Euler angles in radians
    glm::vec3 scale;
};

struct iCamera {
    float fov;       // 视场角
    float nearPlane; // 近裁剪面
    float farPlane;  // 远裁剪面
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
    int32_t modelId; // 使用ID而不是字符串路径，避免marshalling问题
};

struct Material {
    glm::vec3 color;
    float metallic     = 0.0;
    float roughness    = 0.5;
    float occlusion    = 1.0;
    glm::vec3 emissive = glm::vec3(0.0f);
};

struct GameStats {
    int32_t killCount;   // 击杀数量
    float gameTime;      // 游戏时间（秒）
};

class Components {
  public:
    Transform transform;
    Velocity velocity;
    Player player;
    Mesh mesh;
    Material material;
    GameStats gameStats;
    // 可以添加更多组件
};
