// Model.cpp
#include "Model.hpp"

#include "app-context/VulkanApplicationContext.hpp"
#include "utils/logger/Logger.hpp"

Model::Model(VulkanApplicationContext *appContext, Logger *logger, const std::string &filePath)
    : _appContext(appContext), _logger(logger) {
    auto attrsOpt = ModelLoader::loadModelFromPath(filePath, logger);
    if (!attrsOpt.has_value()) {
        logger->error("Failed to load model: {}", filePath);
        return;
    }
    auto attrs = attrsOpt.value();

    for (const auto& mesh : attrs.meshes) {
        vertices.push_back(mesh.vertices);
        indices.push_back(mesh.indices);

        auto vb = std::make_shared<Buffer>(appContext, mesh.vertices.size() * sizeof(Vertex),
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, MemoryStyle::kDedicated);
        vb->fillData(mesh.vertices.data());
        vertexBuffers.push_back(vb);

        auto ib = std::make_shared<Buffer>(appContext, mesh.indices.size() * sizeof(uint32_t),
                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT, MemoryStyle::kDedicated);
        ib->fillData(mesh.indices.data());
        indexBuffers.push_back(ib);

        vertCnts.push_back(static_cast<uint32_t>(mesh.vertices.size()));
        idxCnts.push_back(static_cast<uint32_t>(mesh.indices.size()));

        baseColorTexturePaths.push_back(mesh.baseColorTexturePath);
        normalTexturePaths.push_back(mesh.normalTexturePath);
        metallicRoughnessTexturePaths.push_back(mesh.metallicRoughnessTexturePath);
        emissiveTexturePaths.push_back(mesh.emissiveTexturePath);
    }
}

Model::~Model() {
    // Cleanup if necessary
}