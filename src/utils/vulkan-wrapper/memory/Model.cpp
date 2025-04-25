#include "Model.hpp"
#include "app-context/VulkanApplicationContext.hpp"

#include <string>

Model::Model(VulkanApplicationContext *appContext, Logger *logger, const std::string &filePath)
    : _appContext(appContext), _logger(logger) {
  ModelAttributes attr    = ModelLoader::loadModelFromPath(filePath, _logger);
  _vertices               = attr.vertices;
  _indices                = attr.indices;
  _vertCnt                = static_cast<uint32_t>(_vertices.size());
  size_t vertexBufferSize = _vertices.size() * sizeof(Vertex);
  _vertexBuffer =
      std::make_shared<Buffer>(_appContext, vertexBufferSize,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               MemoryStyle::kDedicated);
  _vertexBuffer->fillData(_vertices.data());
  _idxCnt                = static_cast<uint32_t>(_indices.size());
  size_t indexBufferSize = _indices.size() * sizeof(uint32_t);
  _indexBuffer           = std::make_shared<Buffer>(
      _appContext, indexBufferSize,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, MemoryStyle::kDedicated);
  _indexBuffer->fillData(_indices.data());
}

Model::~Model() {
  _vertexBuffer = nullptr;
  _indexBuffer  = nullptr;
}
