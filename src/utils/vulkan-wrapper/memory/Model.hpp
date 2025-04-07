#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "utils/model-loader/ModelLoader.hpp"
#include "utils/vulkan-wrapper/memory/Buffer.hpp"

class VulkanApplicationContext;

class Model {
public:
  Model(VulkanApplicationContext *appContext, Logger *logger, const std::string& filePath);
  ~Model();

  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;
  Model(Model &&) = delete;
  Model &operator=(Model &&) = delete;

private:
  VulkanApplicationContext *_appContext;

  std::vector<Vertex> _vertices;
  std::vector<uint32_t> _indices;
  std::shared_ptr<Buffer> _vertexBuffer;
  int _vertCnt;
  std::shared_ptr<Buffer> _indexBuffer;
  int _idxCnt;

  Logger *_logger;
};