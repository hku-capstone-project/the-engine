#pragma once

#include <array>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <optional> // Include for std::optional
#include <string>
#include <vector>

#include "volk.h"

class Logger;

struct Vertex {
    glm::vec3 pos      = glm::vec3();
    glm::vec2 texCoord = glm::vec2();
    glm::vec3 normal   = glm::vec3();
    glm::vec4 tangent  = glm::vec4();

    static inline std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof(Vertex, pos);

        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof(Vertex, texCoord);

        attributeDescriptions[2].binding  = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset   = offsetof(Vertex, normal);

        attributeDescriptions[3].binding  = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[3].offset   = offsetof(Vertex, tangent);

        return attributeDescriptions;
    }

    static inline VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }
};

// 每个子模型的属性
struct SubModel {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string baseColorTexturePath; // baseColor 纹理路径
    std::string emissiveTexturePath;  // 发光贴图路径
    std::string metallicRoughnessTexturePath; // 金属粗糙度贴图路径
    std::string normalTexturePath;    // 法线贴图路径
};

struct ModelAttributes {
    std::vector<SubModel> subModels; // 按子模型分组
};

namespace ModelLoader {
// The function now returns an optional, which will be empty if loading fails.
std::optional<ModelAttributes> loadModelFromPath(const std::string &filePath, Logger *logger);
}; // namespace ModelLoader

// ModelAttributes loadModelFromPath(const std::string &filePath, Logger *logger);
// };