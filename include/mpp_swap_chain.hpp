#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_frame_info.hpp"
#include "mpp_checks.hpp"

#include <vector>
#include <memory>

namespace mpp
{

constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

class MppSwapChain 
{
private:
    MppDevice &mppDevice;
    MppWindow &mppWindow;
    VkExtent2D swapchainExtent;

    VkSwapchainKHR swapchain{ VK_NULL_HANDLE };// swapchain
    std::shared_ptr<MppSwapChain> oldSwapChain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkImage depthImage;
    VmaAllocation depthImageAllocation;
    VkImageView depthImageView;

    struct ShaderDataBuffer {
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocationInfo{};
        VkBuffer buffer{ VK_NULL_HANDLE };
        VkDeviceAddress deviceAddress{};
    };
    std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers;
    

    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> fences;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAcquiredSemaphores;
    std::vector<VkSemaphore> renderCompleteSemaphores;

    const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB };
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
    
    uint32_t frameIndex{0};

    uint32_t imageCount{ 0 };

    void createSwapChain();
    void createImageViews();
    void createDepthAttachment();
    void createShaderDataBuffers();
    void createSynchronizationObjects();
    void createCommandBuffers();

    VkSwapchainCreateInfoKHR swapchainCI{};
    VkSemaphoreCreateInfo semaphoreCI{};
    VkImageCreateInfo depthImageCI{};

public:
    MppSwapChain(MppDevice &device, MppWindow &window);
    void recreate();
    ~MppSwapChain();

    const VkFormat& getImageFormat(){return imageFormat;}
    VkFormat getDepthFormat(){return depthFormat;}

    VkResult acquireNextImage(uint32_t& imageIndex);
    VkResult submitCommandBuffer(VkCommandBuffer cb,uint32_t& imageIndex);


    ShaderDataBuffer& getCurrentShaderDataBuffer(){return shaderDataBuffers[frameIndex];}
    VkCommandBuffer getCurrentCommandBuffer(){return commandBuffers[frameIndex];}

    VkImage getCurrentSwapchainImage(uint32_t& imageIndex){return swapchainImages[imageIndex];}
    VkImageView getCurrentSwapchainImageView(uint32_t& imageIndex){return swapchainImageViews[imageIndex];}
    VkImage getDepthImage(){return depthImage;}
    VkImageView getDepthImageView(){return depthImageView;}

};



} // namespace mpp


