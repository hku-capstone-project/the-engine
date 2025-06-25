#include "ModelManager.hpp"
#include "app-context/VulkanApplicationContext.hpp"
#include "utils/logger/Logger.hpp"

ModelManager::ModelManager(VulkanApplicationContext* appContext, Logger* logger)
    : _appContext(appContext), _logger(logger) {
}

int32_t ModelManager::loadModel(const std::string& modelPath) {
    try {
        auto model = std::make_unique<Model>(_appContext, _logger, modelPath);
        
        int32_t modelId = _nextModelId++;
        _models[modelId] = std::move(model);
        
        _logger->info("ModelManager: Loaded model '{}' with ID {}", modelPath, modelId);
        return modelId;
        
    } catch (const std::exception& e) {
        _logger->error("ModelManager: Failed to load model '{}': {}", modelPath, e.what());
        return -1; // 返回无效ID表示加载失败
    }
}

Model* ModelManager::getModel(int32_t modelId) const {
    auto it = _models.find(modelId);
    if (it != _models.end()) {
        return it->second.get();
    }
    return nullptr;
}

void ModelManager::setModelMaterial(int32_t modelId, const MaterialInfo& material) {
    // 预留：将来可以验证modelId是否存在
    _materials[modelId] = material;
    _logger->info("ModelManager: Set material for model ID {}", modelId);
}

std::optional<MaterialInfo> ModelManager::getModelMaterial(int32_t modelId) const {
    auto it = _materials.find(modelId);
    if (it != _materials.end()) {
        return it->second;
    }
    return std::nullopt;
} 