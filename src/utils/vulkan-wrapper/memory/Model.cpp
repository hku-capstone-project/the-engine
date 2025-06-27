#include "Model.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "utils/logger/Logger.hpp"

#include <string>

Model::Model(VulkanApplicationContext *appContext, Logger *logger, const std::string &filePath)
    : _appContext(appContext), _logger(logger) {
<<<<<<< HEAD
    std::optional<ModelAttributes> attr = ModelLoader::loadModelFromPath(filePath, _logger);
    if (!attr) {
        _logger->error("Failed to load model from path: {}", filePath);
        return;
    }
    vertices                = attr->vertices;
    indices                 = attr->indices;
    vertCnt                 = static_cast<uint32_t>(vertices.size());
=======
    // 加载模型
    _attributes = ModelLoader::loadModelFromPath(filePath, _logger);

    // 合并所有子模型的顶点和索引
    for (const auto &subModel : _attributes.subModels) {
        size_t vertexOffset = vertices.size();
        vertices.insert(vertices.end(), subModel.vertices.begin(), subModel.vertices.end());
        for (auto idx : subModel.indices) {
            indices.push_back(idx + vertexOffset);
        }
    }

    // 创建整体缓冲区
    vertCnt = static_cast<uint32_t>(vertices.size());
>>>>>>> main_texture_fixed
    size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    vertexBuffer = std::make_shared<Buffer>(
        _appContext, vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryStyle::kDedicated);
    vertexBuffer->fillData(vertices.data());

    idxCnt = static_cast<uint32_t>(indices.size());
    size_t indexBufferSize = indices.size() * sizeof(uint32_t);
    indexBuffer = std::make_shared<Buffer>(
        _appContext, indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        MemoryStyle::kDedicated);
    indexBuffer->fillData(indices.data());

    // 为每个子模型创建独立缓冲区
    for (const auto &subModel : _attributes.subModels) {
        SubModelBuffers buffers;

        buffers.vertCnt = static_cast<uint32_t>(subModel.vertices.size());
        size_t subVertexBufferSize = subModel.vertices.size() * sizeof(Vertex);
        buffers.vertexBuffer = std::make_shared<Buffer>(
            _appContext, subVertexBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            MemoryStyle::kDedicated);
        buffers.vertexBuffer->fillData(subModel.vertices.data());

        buffers.idxCnt = static_cast<uint32_t>(subModel.indices.size());
        size_t subIndexBufferSize = subModel.indices.size() * sizeof(uint32_t);
        buffers.indexBuffer = std::make_shared<Buffer>(
            _appContext, subIndexBufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            MemoryStyle::kDedicated);
        buffers.indexBuffer->fillData(subModel.indices.data());

        subModelBuffers.push_back(buffers);
        // _logger->info("SubModel: Vertices = {}, Indices = {}", buffers.vertCnt, buffers.idxCnt);
    }
}

Model::~Model() {
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
    for (auto &buffers : subModelBuffers) {
        buffers.vertexBuffer = nullptr;
        buffers.indexBuffer = nullptr;
    }
    subModelBuffers.clear();
}