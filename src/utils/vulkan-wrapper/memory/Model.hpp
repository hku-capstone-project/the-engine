#pragma once
#include <memory>
#include <string>
#include <vector>

#include "utils/model-loader/ModelLoader.hpp"
#include "utils/vulkan-wrapper/memory/Buffer.hpp"
#include <functional> // For std::function

class VulkanApplicationContext;
class Logger;

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

    // 每个子模型的缓冲区
    struct SubModelBuffers {
        std::shared_ptr<Buffer> vertexBuffer;
        std::shared_ptr<Buffer> indexBuffer;
        uint32_t vertCnt;
        uint32_t idxCnt;
    };
    std::vector<SubModelBuffers> subModelBuffers; // 存每个子模型的缓冲区

    const std::optional<ModelAttributes>& getModelAttributes() const { return _attributes; }

private:
    VulkanApplicationContext *_appContext;
    Logger *_logger;
    std::optional<ModelAttributes> _attributes; // 存储加载的模型数据
};