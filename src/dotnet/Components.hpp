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
    
    // Comparison operator for map key
    bool operator<(const Material& other) const {
        if (color.x != other.color.x) return color.x < other.color.x;
        if (color.y != other.color.y) return color.y < other.color.y;
        if (color.z != other.color.z) return color.z < other.color.z;
        if (metallic != other.metallic) return metallic < other.metallic;
        if (roughness != other.roughness) return roughness < other.roughness;
        if (occlusion != other.occlusion) return occlusion < other.occlusion;
        if (emissive.x != other.emissive.x) return emissive.x < other.emissive.x;
        if (emissive.y != other.emissive.y) return emissive.y < other.emissive.y;
        return emissive.z < other.emissive.z;
    }
};

struct InstanceData {
    glm::mat4 modelMatrix;
    int32_t materialIndex;
    float padding1;
    float padding2;
    float padding3;
};

class Components {
  public:
    Transform transform;
    Velocity velocity;
    Player player;
    Mesh mesh;
    Material material;
    // 可以添加更多组件
};
