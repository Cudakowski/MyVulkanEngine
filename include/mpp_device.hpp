#pragma once

#include "mpp_window.hpp"
#include "mpp_checks.hpp"

// libs
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vma/vk_mem_alloc.h>

// std
#include <vector>
#include <set>
#include <iostream>
#include <cassert>

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
    bool isComplete()const { return graphicsFamilyHasValue && presentFamilyHasValue; }
};

class MppDevice 
{
private:
    MppWindow& mppWindow;

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
    bool checkValidationLayerSupport()const;
    void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT &createInfo)const;
    VkDebugUtilsMessengerEXT debugMessenger;
    bool isDeviceSuitable(VkPhysicalDevice device)const;
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device)const;
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)const;

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

    VkInstance getInstance()const{return instance;}
    VkPhysicalDevice getPhysicalDevice()const{return physicalDevice;}
    VkDevice getLogicalDevice()const{return device_;}
    VmaAllocator getAllocator()const{return allocator;}
    VkCommandPool getRenderingCommandPool()const{return renderingCommandPool;}
    VkCommandPool getTextureLoadingCommandPool()const{return textureLoadingCommandPool;}
    SwapChainSupportDetails getSwapChainSupport() const{ return querySwapChainSupport(physicalDevice); }
    VkSurfaceKHR getSurface()const{return surface;}
    VkQueue getGraphicsQueue()const{return graphicsQueue;}
    VkQueue getPresentQueue()const{return presentQueue;}

    VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    QueueFamilyIndices findPhysicalQueueFamilies() const{ return findQueueFamilies(physicalDevice); }
    VkFormat findSupportedFormat( const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)const;
    VkSurfaceCapabilitiesKHR getDeviceSurfaceCapabilities()const;
};
    
} // namespace mpp

