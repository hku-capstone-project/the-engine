#include "Model.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "utils/logger/Logger.hpp"

#include <string>

Model::Model(VulkanApplicationContext *appContext, Logger *logger, const std::string &filePath)
    : _appContext(appContext), _logger(logger) {
    std::optional<ModelAttributes> attr = ModelLoader::loadModelFromPath(filePath, _logger);
    if (!attr) {
        _logger->error("Failed to load model from path: {}", filePath);
        return;
    }
    vertices                = attr->vertices;
    indices                 = attr->indices;
    vertCnt                 = static_cast<uint32_t>(vertices.size());
    size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    vertexBuffer            = std::make_shared<Buffer>(_appContext, vertexBufferSize,
                                                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                       MemoryStyle::kDedicated);
    vertexBuffer->fillData(vertices.data());
    idxCnt                 = static_cast<uint32_t>(indices.size());
    size_t indexBufferSize = indices.size() * sizeof(uint32_t);
    indexBuffer            = std::make_shared<Buffer>(_appContext, indexBufferSize,
                                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                      MemoryStyle::kDedicated);
    indexBuffer->fillData(indices.data());
}

Model::~Model() {
    vertexBuffer = nullptr;
    indexBuffer  = nullptr;
}
