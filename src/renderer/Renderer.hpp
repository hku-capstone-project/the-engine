#pragma once
#define VK_NO_PROTOTYPES

#include "vma/vk_mem_alloc.h"
#include "volk.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "utils/vulkan-wrapper/pipeline/GfxPipeline.hpp"

class VulkanApplicationContext;
class Logger;
class ShaderCompiler;
class Window;
class ConfigContainer;
class Model;
class Image;
class ImageForwardingPair;

class Camera {
public:
    Camera() {
        position = glm::vec3(0.0f, 0.0f, 5.0f);
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
};

class Renderer {
  public:
    Renderer(VulkanApplicationContext *appContext, Logger *logger, ShaderCompiler *shaderCompiler,
             Window *window, ConfigContainer *configContainer);
    ~Renderer();

    // disable move and copy
    Renderer(const Renderer &)            = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer(Renderer &&)                 = delete;
    Renderer &operator=(Renderer &&)      = delete;

    void drawFrame(size_t currentFrame);
    void processInput(double deltaTime);

    void onSwapchainResize();
    [[nodiscard]] inline VkCommandBuffer getTracingCommandBuffer(size_t currentFrame) {
        return _tracingCommandBuffers[currentFrame];
    }

    [[nodiscard]] inline VkCommandBuffer getDeliveryCommandBuffer(size_t imageIndex) {
        return _deliveryCommandBuffers[imageIndex];
    }

  private:
    VulkanApplicationContext *_appContext;
    Logger *_logger;
    ShaderCompiler *_shaderCompiler;
    Window *_window;
    ConfigContainer *_configContainer;
    std::unique_ptr<GfxPipeline> _pipeline = nullptr;

    std::vector<VkCommandBuffer> _deliveryCommandBuffers{};
    std::vector<VkCommandBuffer> _tracingCommandBuffers{};
    std::vector<VkFramebuffer> _frameBuffers{};
    std::unique_ptr<Model> _model = nullptr;
    struct {
        std::unique_ptr<Image> baseColor;
        std::unique_ptr<Image> normalMap;
        std::unique_ptr<Image> metalRoughness;
    } _images;

    VkRenderPass _renderPass;

    std::unique_ptr<Image> _renderTargetImage = nullptr;
    struct {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
    } depthStencil;

    struct {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
    } colorResources;

    Camera _camera;
    MVP _mvp;
    float _rotation = 0.0f;

    void _recordDeliveryCommandBuffers();
    void _recordTracingCommandBuffers();
    void _createRenderPass();
    void _createFrameBuffers();
    void _createDepthStencil();
    void _createColorResources();
};
