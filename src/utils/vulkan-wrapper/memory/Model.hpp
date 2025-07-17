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

    std::vector<std::vector<Vertex>> vertices;
    std::vector<std::vector<uint32_t>> indices;
    std::vector<std::shared_ptr<Buffer>> vertexBuffers;
    std::vector<uint32_t> vertCnts;
    std::vector<std::shared_ptr<Buffer>> indexBuffers;
    std::vector<uint32_t> idxCnts;
    
    std::vector<std::string> baseColorTexturePaths;
    std::vector<std::string> normalTexturePaths;
    std::vector<std::string> metallicRoughnessTexturePaths;
    std::vector<std::string> emissiveTexturePaths;

  private:
    VulkanApplicationContext *_appContext;

    Logger *_logger;
};