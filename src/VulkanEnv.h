#pragma once


#include <vulkan/vulkan.h>

#include <vector>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

class VulkanEnv 
{
public:
    VulkanEnv() = default;
    virtual ~VulkanEnv() = default;

    void init(void* nwh);
    void clean();

protected:
    
    VkSampleCountFlagBits getMaxUsableSampleCount() const;

    /**
     * @brief Copies data from one Vulkan buffer to another.
     * 
     * This function performs a copy operation from the source buffer to the 
     * destination buffer using Vulkan commands. It is typically used to 
     * transfer data between buffers, such as staging buffers and device-local 
     * buffers.
     * 
     * @param srcBuffer The source Vulkan buffer containing the data to copy.
     * @param dstBuffer The destination Vulkan buffer where the data will be copied.
     * @param size The size of the data to copy, in bytes.
     */
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

    virtual void createFramebuffers() = 0;
    virtual void cleanupFramebuffers() = 0;

    virtual void cleanupDepthImage() = 0;
    virtual void createDepthImage() = 0;

    virtual void cleanupColorResources() = 0;
    virtual void createColorResources() = 0;

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    
    void transitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image, 
        VkFormat format, 
        VkImageAspectFlags aspectFlags,
        VkAccessFlags srcAccessMask, 
        VkAccessFlags dstAccessMask, 
        VkPipelineStageFlags srcStageMask, 
        VkPipelineStageFlags dstStageMask, 
        VkImageLayout oldLayout, 
        VkImageLayout newLayout,
        uint32_t mipLevels);
    void copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    
    void genMipmaps(VkCommandBuffer commandBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels);

    void allocateCommandBuffer(VkCommandBuffer& commandBuffer, VkCommandBufferLevel level);

    void startCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags);
    void endOneTimeCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);

    void createTextureSampler(VkSampler& sampler);
protected:
    void createInstance();

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) ;

    void setupDebugMessenger() ;

    void pickPhysicalDevice() ;
    bool isDeviceSuitable(VkPhysicalDevice device) ;

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    void createLogicDevice();

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsAndComputeQueueFamily;
        std::optional<uint32_t> presentQueueFamily;
        
        bool isComplete() {
            return graphicsAndComputeQueueFamily.has_value() && presentQueueFamily.has_value();
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    struct SurfaceDetails{
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    SurfaceDetails querySurfaceSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const SurfaceDetails & details);
    VkPresentModeKHR chooseSwapSurfacePresentMode(const SurfaceDetails & details);
    VkExtent2D chooseSwapExtent(const SurfaceDetails & details);

    void createSwapchain();   
    void cleanupSwapchain();
    void recreateSwapchain();

    
    void createCommandPool();

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,  VkMemoryPropertyFlags flags, VkBuffer& buffer, VkDeviceMemory& memory);

    void createSyncObject();

    VkShaderModule createShaderModule(const std::vector<char> & shaderCode);

    std::vector<const char*> getRequiredExtensions() ;
    bool checkValidationLayerSupport() ;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) ;

    
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) ;

    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) ;

    static std::vector<char> readFile(const std::string& fn);

protected:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkSurfaceKHR m_surface;
    VkSampleCountFlagBits m_maxSampleCount;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_logicDevice;
    VkQueue m_graphicsAndComputeQueue;
    VkQueue m_presentQueue;
    VkSwapchainKHR m_swapchain;
    std::vector<VkImage> m_swapchainsImages;
    VkFormat m_format;
    VkExtent2D m_extent;
    std::vector<VkImageView> m_imageViews;

    VkCommandPool m_commandPool;
    
    std::vector<VkSemaphore> m_imageAvailabelesemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFence;
    
    void* m_nwh = {};
    int m_curFrame = 0;
    VkFormat m_GBufferImageFormat = VK_FORMAT_UNDEFINED;
};
