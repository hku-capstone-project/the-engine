#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <optional>

#include "utils/vulkan-wrapper/memory/Model.hpp"

class VulkanApplicationContext;
class Logger;

// 预留材质信息结构体
struct MaterialInfo {
    std::string baseColorTexture = "";
    std::string normalTexture = "";
    std::string metallicRoughnessTexture = "";
    // 可以扩展更多材质属性
};

class ModelManager {
public:
    ModelManager(VulkanApplicationContext* appContext, Logger* logger);
    ~ModelManager() = default;

    // 禁用拷贝和移动
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;
    ModelManager(ModelManager&&) = delete;
    ModelManager& operator=(ModelManager&&) = delete;

    // 加载模型并返回模型ID
    int32_t loadModel(const std::string& modelPath);
    
    // 根据ID获取模型（如果不存在返回nullptr）
    Model* getModel(int32_t modelId) const;
    
    // 预留：设置模型的材质信息
    void setModelMaterial(int32_t modelId, const MaterialInfo& material);
    
    // 预留：获取模型的材质信息
    std::optional<MaterialInfo> getModelMaterial(int32_t modelId) const;
    
    // 获取当前加载的模型数量
    size_t getModelCount() const { return _models.size(); }

private:
    VulkanApplicationContext* _appContext;
    Logger* _logger;
    
    // 模型ID到模型的映射
    std::unordered_map<int32_t, std::unique_ptr<Model>> _models;
    
    // 预留：模型ID到材质信息的映射
    std::unordered_map<int32_t, MaterialInfo> _materials;
    
    // 下一个可用的模型ID
    int32_t _nextModelId = 0;
}; 