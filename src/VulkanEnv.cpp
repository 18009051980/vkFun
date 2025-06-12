#include "VulkanEnv.h"

#include "FramebufferSizeUtil.h"


#include <windows.h>              
#include <vulkan/vulkan_win32.h>

#include <limits>
#include <algorithm>
#include <fstream>



#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

uint32_t VulkanEnv::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if (typeFilter & (1<<i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanEnv::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    if (VK_SUCCESS != vkCreateBuffer(m_logicDevice, &createInfo, nullptr, &buffer))
    {
        std::cerr << "创建顶点缓冲失败"<< std::endl;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(m_logicDevice, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, flags)
    };

    if (VK_SUCCESS != vkAllocateMemory(m_logicDevice, &allocInfo, nullptr, &memory))
    {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    vkBindBufferMemory(m_logicDevice, buffer, memory, 0);
    
}

void VulkanEnv::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    
    VkCommandBuffer commandBuffer{};
    allocateCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    startCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  
    VkBufferCopy copyRegion{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endOneTimeCommandBuffer(commandBuffer, m_graphicsAndComputeQueue);
}



void VulkanEnv::init(void* nwh)
{
    m_nwh = nwh;
    
    createInstance();
    setupDebugMessenger();
    

    m_surface = {};
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)m_nwh;
    createInfo.hinstance = GetModuleHandle(nullptr);

    if (VK_SUCCESS != vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_surface))
    {
        throw std::runtime_error("failed to create surface!");
    }
    pickPhysicalDevice();
    createLogicDevice();
    createSwapchain();

    m_imageViews.resize(m_swapchainsImages.size());
    for (size_t i = 0; i < m_swapchainsImages.size(); i++)
    {
        m_imageViews[i] = createImageView(m_swapchainsImages[i], m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
       
    
    createCommandPool();
    
    createSyncObject();
}   

void VulkanEnv::clean()
{
    auto surface = m_surface;
    {

        cleanupSwapchain();

        for (size_t i = 0; i < 2; i++)
        {
            vkDestroySemaphore(m_logicDevice, m_imageAvailabelesemaphores[i], nullptr);
            vkDestroySemaphore(m_logicDevice, m_renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_logicDevice, m_inFlightFence[i], nullptr);
        }
        
        vkDestroyCommandPool(m_logicDevice, m_commandPool, nullptr);

        vkDestroyDevice(m_logicDevice, nullptr);
        
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    if(enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
    
}

void VulkanEnv::createInstance() 
{
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void VulkanEnv::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

void VulkanEnv::setupDebugMessenger() 
{
    if (!enableValidationLayers) return;
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanEnv::pickPhysicalDevice() 
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            m_physicalDevice = device;
            m_maxSampleCount = getMaxUsableSampleCount();
            break;
        }
    }
    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool VulkanEnv::isDeviceSuitable(VkPhysicalDevice device) 
{
    VkFormat colorFormat[] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT};
    for (auto format : colorFormat)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device, format, &props);
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
        {
            m_GBufferImageFormat = format;
            break;
        }
    }


    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SurfaceDetails surfaceSupport = querySurfaceSupport(device);
        swapChainAdequate = !surfaceSupport.formats.empty() && !surfaceSupport.presentModes.empty();
    }
    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
       deviceFeatures.geometryShader && indices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy && m_GBufferImageFormat != VK_FORMAT_UNDEFINED;
}

bool VulkanEnv::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t deviceExtensionCount = {};
    vkEnumerateDeviceExtensionProperties(device, nullptr, & deviceExtensionCount, nullptr);

    std::vector<VkExtensionProperties> deviceExts(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, deviceExts.data());

    std::unordered_set<std::string> requiedExts(deviceExtensions.begin(), deviceExtensions.end());

    for(auto & ext : deviceExts)
    {
        requiedExts.erase(ext.extensionName);
    }

    return requiedExts.empty();
}


VulkanEnv::QueueFamilyIndices VulkanEnv::findQueueFamilies(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    
    int i = 0;
    for (const auto& queueFamily : queueFamilies) 
    {
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.graphicsAndComputeQueueFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if(presentSupport)
        {
            indices.presentQueueFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
}

std::vector<const char*> VulkanEnv::getRequiredExtensions() 
{
    std::vector<const char*> extensions{"VK_KHR_surface", "VK_KHR_win32_surface"};
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

VulkanEnv::SurfaceDetails VulkanEnv::querySurfaceSupport(VkPhysicalDevice device)
{
    SurfaceDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    bool ok = details.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT;
    ok = details.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT;
    
    VkImageFormatProperties formatProps;
    VkResult result = vkGetPhysicalDeviceImageFormatProperties(
        device,
        VK_FORMAT_R8G8B8A8_UNORM,       
        VK_IMAGE_TYPE_2D,             
        VK_IMAGE_TILING_OPTIMAL,       
        VK_IMAGE_USAGE_STORAGE_BIT,    
        0,                             
        &formatProps
    );

    if (result != VK_SUCCESS || formatProps.maxExtent.width == 0) {
        
    }

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, & formatCount, nullptr);

    if (formatCount)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}
VkSurfaceFormatKHR VulkanEnv::chooseSwapSurfaceFormat(const SurfaceDetails & details)
{
    for(auto & format : details.formats)
    {
        if(format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }
    return details.formats[0];
}
VkPresentModeKHR VulkanEnv::chooseSwapSurfacePresentMode(const SurfaceDetails & details)
{
    for (auto & mode : details.presentModes)
    {
        if(mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D VulkanEnv::chooseSwapExtent(const SurfaceDetails & details)
{
    if (details.capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
    {
        return details.capabilities.currentExtent;
    }
    
    
    FramebufferSizeUtil util;
    auto && [w, h] = util.get(m_nwh);
    
    VkExtent2D extent{w, h};
    extent.width = std::clamp(extent.width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);

    return extent;
}



bool VulkanEnv::checkValidationLayerSupport() 
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEnv::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult VulkanEnv::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanEnv::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void VulkanEnv::createLogicDevice()
{
    
    auto indices = findQueueFamilies(m_physicalDevice);
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::unordered_set<uint32_t> queueFamilies = {indices.graphicsAndComputeQueueFamily.value(), indices.presentQueueFamily.value()};


    float queuePriority = 1.0f;
    for(auto queueFamily : queueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsAndComputeQueueFamily.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    VkPhysicalDeviceFeatures deviceFeatures{
        .sampleRateShading = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        .fragmentStoresAndAtomics = VK_TRUE,
    };
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames= deviceExtensions.data();
    if(enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }
    if(VK_SUCCESS != vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_logicDevice))
    {
        throw std::runtime_error("failed to create logic device");
    }
    vkGetDeviceQueue(m_logicDevice, indices.graphicsAndComputeQueueFamily.value(), 0, &m_graphicsAndComputeQueue);
    vkGetDeviceQueue(m_logicDevice, indices.presentQueueFamily.value(), 0, &m_presentQueue);
}
void VulkanEnv::createSwapchain() 
{
    SurfaceDetails details = querySurfaceSupport(m_physicalDevice);

    VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(details);
    VkPresentModeKHR mode = chooseSwapSurfacePresentMode(details);
    VkExtent2D extent = chooseSwapExtent(details);

    m_format = format.format;
    m_extent = extent;

    uint32_t imageCount = details.capabilities.minImageCount + 1;
    
    imageCount = (imageCount > details.capabilities.maxImageCount) ? details.capabilities.maxImageCount : imageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = format.format;
    createInfo.imageColorSpace = format.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeQueueFamily.value(), indices.presentQueueFamily.value()};

    if (indices.graphicsAndComputeQueueFamily != indices.presentQueueFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }
    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (VK_SUCCESS != vkCreateSwapchainKHR(m_logicDevice, &createInfo, nullptr, &m_swapchain))
    {
        throw std::runtime_error("failed to create swapchain");
    }
    vkGetSwapchainImagesKHR(m_logicDevice, m_swapchain, &imageCount, nullptr);
    m_swapchainsImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_logicDevice, m_swapchain, &imageCount, m_swapchainsImages.data());
}

void VulkanEnv::cleanupSwapchain()
{
    cleanupFramebuffers();
    cleanupDepthImage();
    cleanupColorResources();

    for (int i = 0; i < m_imageViews.size(); ++i)
    {
        vkDestroyImageView(m_logicDevice, m_imageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(m_logicDevice, m_swapchain, nullptr);
}

void VulkanEnv::recreateSwapchain()
{
    vkDeviceWaitIdle(m_logicDevice);

    cleanupSwapchain();

    createSwapchain();

    m_imageViews.resize(m_swapchainsImages.size());
    for (size_t i = 0; i < m_swapchainsImages.size(); i++)
    {
        m_imageViews[i] = createImageView(m_swapchainsImages[i], m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
    
    createColorResources();
    createDepthImage();
    createFramebuffers();
}

VkImageView VulkanEnv::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageView imageView;

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectFlags;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = mipLevels;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;
    if (VK_SUCCESS != vkCreateImageView(m_logicDevice, &createInfo, nullptr, &imageView))
    {
        throw std::runtime_error("failed to create image view");
    }
    
    return imageView;
}

void VulkanEnv::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeQueueFamily.value();
    if (vkCreateCommandPool(m_logicDevice, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void VulkanEnv::createSyncObject()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    m_imageAvailabelesemaphores.resize(2);
    m_renderFinishedSemaphores.resize(2);
    m_inFlightFence.resize(2);
    for (int i = 0; i < 2; ++i)
    {
        if (vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_imageAvailabelesemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(m_logicDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(m_logicDevice, &fenceInfo, nullptr, &m_inFlightFence[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores!");
        }
    }
    

}


std::vector<char> VulkanEnv::readFile(const std::string & fn)
{
    std::ifstream file(fn, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule VulkanEnv::createShaderModule(const std::vector<char> & shaderCode)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule{};
    if(VK_SUCCESS != vkCreateShaderModule(m_logicDevice, &createInfo, nullptr, &shaderModule))
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}


void VulkanEnv::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = numSamples,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    
    vkCreateImage(m_logicDevice, &imageInfo, nullptr, &image);

    VkMemoryRequirements imageMemRequirements;
    vkGetImageMemoryRequirements(m_logicDevice, image, &imageMemRequirements);

    VkMemoryAllocateInfo alloInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemRequirements.size,
        .memoryTypeIndex = findMemoryType(imageMemRequirements.memoryTypeBits, properties),
    };

    vkAllocateMemory(m_logicDevice, &alloInfo, nullptr, &imageMemory);

    vkBindImageMemory(m_logicDevice, image, imageMemory,0);
}
   
void VulkanEnv::startCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags,

    };
    if (VK_SUCCESS != vkBeginCommandBuffer(commandBuffer, & beginInfo))
    {
        throw std::runtime_error("failed to begin command buffer!");
    }
}
void VulkanEnv::endOneTimeCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
{
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    vkQueueSubmit(m_graphicsAndComputeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsAndComputeQueue);

    vkFreeCommandBuffers(m_logicDevice, m_commandPool, 1, &commandBuffer);
}

void VulkanEnv::allocateCommandBuffer(VkCommandBuffer& commandBuffer, VkCommandBufferLevel level)
{
    VkCommandBufferAllocateInfo allocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .level = level,
        .commandBufferCount = 1,
    };
    if (VK_SUCCESS != vkAllocateCommandBuffers(m_logicDevice, &allocInfo, &commandBuffer))
    {
        throw std::runtime_error("failed to allocate command buffer!");
    } 

}

void VulkanEnv::transitionImageLayout(
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
    uint32_t mipLevels)
{
    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = 1
            },
    };
    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanEnv::copyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1},
    };
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void VulkanEnv::createTextureSampler(VkSampler& sampler)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    if (VK_SUCCESS != vkCreateSampler(m_logicDevice, &samplerInfo, nullptr, &sampler))
    {
        throw std::runtime_error("failed to create texture sampler");
    }
}

void VulkanEnv::genMipmaps(VkCommandBuffer commandBuffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels)
{
    int texWid = width;
    int texHei = height;

    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
    };
    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.subresourceRange.baseMipLevel = i - 1;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        VkImageBlit blit{
            .srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets = {
                {0, 0, 0},
                {texWid, texHei, 1},
            },
            .dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .dstOffsets = {
                {0, 0, 0},
                {texWid > 1 ? texWid / 2 : 1, texHei > 1 ? texHei / 2 : 1, 1},
            },
        };
        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        if (texWid > 1)
        {
            texWid = texWid / 2;
        }
        if (texHei > 1)
        {
            texHei = texHei / 2;
        }
    }
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
VkSampleCountFlagBits VulkanEnv::getMaxUsableSampleCount() const
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);
    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}
