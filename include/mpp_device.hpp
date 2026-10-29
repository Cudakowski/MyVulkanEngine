#pragma once

#include "mpp_window.hpp"

#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <vector>
#include <set>

namespace mpp
{
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
    
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    bool graphicsFamilyHasValue = false;
    bool presentFamilyHasValue = false;
    bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class MppDevice 
{
private:
    MppWindow& mppWindow;

    void checkAssets();
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void detectingDevice();
    void deviceSetup();
    void vmaSetup();
    void createCommandPool();

    VkInstance instance{ VK_NULL_HANDLE }; //instancja, po najechaniu myszką można zobaczyć że to wskaźnik inicjalizowany nullptr'em
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceIndex{0};// defaultowo 0

    VkDevice device_{ VK_NULL_HANDLE };// logical device
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentQueue;// kolejka z komendami(cammand buffer)
    VmaAllocator allocator{ VK_NULL_HANDLE };//alokator
    VkCommandPool renderingCommandPool{ VK_NULL_HANDLE };
    VkCommandPool textureLoadingCommandPool{ VK_NULL_HANDLE };
    
    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    VkDebugUtilsMessengerEXT debugMessenger;
    bool isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

public:
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif
    MppDevice(MppWindow& window);
    ~MppDevice();
    
    MppDevice(const MppDevice&) = delete;
    MppDevice& operator=(const MppDevice&) = delete;
    MppDevice(MppDevice&&) = delete;
    MppDevice& operator=(MppDevice&&) = delete;

    VkInstance getInstance(){return instance;}
    VkPhysicalDevice getPhysicalDevice(){return physicalDevice;}
    VkDevice getLogicalDevice(){return device_;}
    VmaAllocator getAllocator(){return allocator;}
    VkCommandPool getRenderingCommandPool(){return renderingCommandPool;}
    VkCommandPool getTextureLoadingCommandPool(){return textureLoadingCommandPool;}
    SwapChainSupportDetails getSwapChainSupport() { return querySwapChainSupport(physicalDevice); }
    VkSurfaceKHR getSurface(){return surface;}
    VkQueue getGraphicsQueue(){return graphicsQueue;}
    VkQueue getPresentQueue(){return presentQueue;}

    VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    QueueFamilyIndices findPhysicalQueueFamilies() { return findQueueFamilies(physicalDevice); }
    VkFormat findSupportedFormat( const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkSurfaceCapabilitiesKHR getDeviceSurfaceCapabilities();
};
    
} // namespace mpp

