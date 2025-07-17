#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include "VulkanApplicationContext.hpp"

#include "utils/logger/Logger.hpp"

static const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef __APPLE__
static const std::vector<const char *> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                   "VK_KHR_portability_subset"};
#else
static const std::vector<const char *> requiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#endif

VulkanApplicationContext::VulkanApplicationContext() = default;

VulkanApplicationContext::~VulkanApplicationContext() {
    vkDestroyCommandPool(_device, _commandPool, nullptr);
    vkDestroyCommandPool(_device, _guiCommandPool, nullptr);

    for (auto &swapchainImageView : _swapchainImageViews) {
        vkDestroyImageView(_device, swapchainImageView, nullptr);
    }

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    vkDestroySurfaceKHR(_vkInstance, _surface, nullptr);

    // this step destroys allocated VkDestroyMemory allocated by VMA when creating
    // buffers and images, by destroying the global allocator
    vmaDestroyAllocator(_allocator);
    vkDestroyDevice(_device, nullptr);

#ifndef NVALIDATIONLAYERS
    vkDestroyDebugUtilsMessengerEXT(_vkInstance, _debugMessager, nullptr);
#endif // NDEBUG

    vkDestroyInstance(_vkInstance, nullptr);
}

void VulkanApplicationContext::init(Logger *logger, GLFWwindow *window,
                                    GraphicsSettings *settings) {
    _logger = logger;
    _logger->info("Initiating VulkanApplicationContext");
#ifndef NVALIDATIONLAYERS
    _logger->info("Validation layers are enabled");
#else
    _logger->info("Validation layers are disabled");
#endif // NVLIDATIONLAYERS

    _glWindow = window;
    volkInitialize();

    VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName   = "The engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_2;
    ContextCreator::createInstance(_logger, _vkInstance, _debugMessager, appInfo, validationLayers);

    ContextCreator::createSurface(_logger, _vkInstance, _surface, _glWindow);

    // selects physical device, creates logical device from that, decides queues,
    // loads device-related functions too
    ContextCreator::QueueSelection queueSelection{};
    ContextCreator::createDevice(_logger, _physicalDevice, _device, _queueFamilyIndices,
                                 queueSelection, _vkInstance, _surface, requiredDeviceExtensions);
    _graphicsQueueIndex = queueSelection.graphicsQueueIndex;
    _presentQueueIndex  = queueSelection.presentQueueIndex;
    _computeQueueIndex  = queueSelection.computeQueueIndex;
    _transferQueueIndex = queueSelection.transferQueueIndex;

    _graphicsQueue = queueSelection.graphicsQueue;
    _presentQueue  = queueSelection.presentQueue;
    _computeQueue  = queueSelection.computeQueue;
    _transferQueue = queueSelection.transferQueue;

    _msaaSamples                     = queueSelection.msaaSamples;
    std::vector<VkFormat> formatList = {
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D16_UNORM,
    };

    for (auto format : formatList) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &properties);
        if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            _depthFormat = format;
            break;
        }
    }

    _createSwapchain(settings->isFramerateLimited);
    _createAllocator();
    _createCommandPool();
}

void VulkanApplicationContext::onSwapchainResize(bool isFramerateLimited) {
    for (auto &swapchainImageView : _swapchainImageViews) {
        vkDestroyImageView(_device, swapchainImageView, nullptr);
    }
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    _createSwapchain(isFramerateLimited);
}

void VulkanApplicationContext::_createSwapchain(bool isFramerateLimited) {
    ContextCreator::createSwapchain(_logger, isFramerateLimited, _swapchain, _swapchainImages,
                                    _swapchainImageViews, _swapchainSurfaceFormat, _swapchainExtent,
                                    _surface, _device, _physicalDevice, _queueFamilyIndices);
}

void VulkanApplicationContext::_createAllocator() {
    // load vulkan functions dynamically
    VmaVulkanFunctions vmaVulkanFunc{};
    vmaVulkanFunc.vkAllocateMemory                        = vkAllocateMemory;
    vmaVulkanFunc.vkBindBufferMemory                      = vkBindBufferMemory;
    vmaVulkanFunc.vkBindImageMemory                       = vkBindImageMemory;
    vmaVulkanFunc.vkCreateBuffer                          = vkCreateBuffer;
    vmaVulkanFunc.vkCreateImage                           = vkCreateImage;
    vmaVulkanFunc.vkDestroyBuffer                         = vkDestroyBuffer;
    vmaVulkanFunc.vkDestroyImage                          = vkDestroyImage;
    vmaVulkanFunc.vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges;
    vmaVulkanFunc.vkFreeMemory                            = vkFreeMemory;
    vmaVulkanFunc.vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements;
    vmaVulkanFunc.vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements;
    vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties;
    vmaVulkanFunc.vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties;
    vmaVulkanFunc.vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges;
    vmaVulkanFunc.vkMapMemory                             = vkMapMemory;
    vmaVulkanFunc.vkUnmapMemory                           = vkUnmapMemory;
    vmaVulkanFunc.vkCmdCopyBuffer                         = vkCmdCopyBuffer;
    vmaVulkanFunc.vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2;
    vmaVulkanFunc.vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2;
    vmaVulkanFunc.vkBindBufferMemory2KHR                  = vkBindBufferMemory2;
    vmaVulkanFunc.vkBindImageMemory2KHR                   = vkBindImageMemory2;
    vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
    vmaVulkanFunc.vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements;
    vmaVulkanFunc.vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
    allocatorInfo.physicalDevice         = _physicalDevice;
    allocatorInfo.device                 = _device;
    allocatorInfo.instance               = _vkInstance;
    allocatorInfo.pVulkanFunctions       = &vmaVulkanFunc;

    vmaCreateAllocator(&allocatorInfo, &_allocator);
}

// create a command pool for rendering commands and a command pool for gui
// commands (imgui)
void VulkanApplicationContext::_createCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.queueFamilyIndex = _queueFamilyIndices.graphicsFamily;
    // this flag allows the use of vkResetCommandBuffer
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_commandPool);
    vkCreateCommandPool(_device, &commandPoolCreateInfo, nullptr, &_guiCommandPool);
}

VkCommandBuffer VulkanApplicationContext::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanApplicationContext::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(_graphicsQueue);

    vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}
