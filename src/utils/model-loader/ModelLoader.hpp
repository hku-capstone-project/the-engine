#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "volk.h"

class Logger;

struct Vertex {
  glm::vec3 pos = glm::vec3();
  glm::vec2 texCoord = glm::vec2();
  glm::vec3 normal = glm::vec3();
  glm::vec4 tangent = glm::vec4();

  static inline std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions();
  static inline VkVertexInputBindingDescription GetBindingDescription();
};

struct ModelAttributes {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

namespace ModelLoader {
ModelAttributes loadModelFromPath(const std::string &filePath, Logger *logger);
};
