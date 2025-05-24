#pragma once
#include <memory>
#include <string>
#include <vector>

#include "utils/model-loader/ModelLoader.hpp"
#include "utils/vulkan-wrapper/memory/Buffer.hpp"

class VulkanApplicationContext;

class Model {
public:
  Model(VulkanApplicationContext *appContext, Logger *logger, const std::string &filePath);
  ~Model();

  Model(const Model &)            = delete;
  Model &operator=(const Model &) = delete;
  Model(Model &&)                 = delete;
  Model &operator=(Model &&)      = delete;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::shared_ptr<Buffer> vertexBuffer;
  uint32_t vertCnt;
  std::shared_ptr<Buffer> indexBuffer;
  uint32_t idxCnt;

private:
  VulkanApplicationContext *_appContext;

  Logger *_logger;
};
