#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_frame_info.hpp"
#include "mpp_checks.hpp"

#include <vector>
#include <memory>

namespace mpp
{

class MppSwapChain 
{
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT{2};
private:
    MppDevice &mppDevice;
    VkExtent2D windowExtent;
    VkExtent2D swapchainExtent;

    VkSwapchainKHR swapchain{ VK_NULL_HANDLE };// swapchain
    std::shared_ptr<MppSwapChain> oldSwapChain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    // VkImage depthImage;
    // VmaAllocation depthImageAllocation;
    // VkImageView depthImageView;
    std::array<VkImage, MAX_FRAMES_IN_FLIGHT> depthImages;
    std::array<VmaAllocation, MAX_FRAMES_IN_FLIGHT> depthImageAllocations;
    std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> depthImageViews;
    VkExtent2D swapChainExtent;

    


    std::vector<VkSemaphore> renderCompleteSemaphores;

    VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB };//TODO nie inicjalizować tylko sprawdzać
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };//TODO nie inicjalizować tylko sprawdzać
    

    uint32_t imageCount{ 0 };

    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthAttachment();
    void createSynchronizationObjects();


    VkFormat findDepthFormat();
    VkSurfaceFormatKHR MppSwapChain::chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR MppSwapChain::chooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D MppSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

public:
    MppSwapChain(MppDevice &device, VkExtent2D windowExtent);
    MppSwapChain(MppDevice &device, VkExtent2D windowExtent,std::shared_ptr<MppSwapChain> previous);
    //void recreate();
    ~MppSwapChain();

    MppSwapChain(const MppSwapChain &) = delete;
    MppSwapChain &operator=(const MppSwapChain &) = delete;

    const VkFormat& getImageFormat(){return imageFormat;}
    VkFormat getDepthFormat(){return depthFormat;}

    VkResult acquireNextImage(uint32_t& imageIndex, VkFence fence, VkSemaphore imageAcquiredSemaphore);
    VkResult submitCommandBuffer(VkCommandBuffer cb,uint32_t& imageIndex, VkFence fence, VkSemaphore imageAcquiredSemaphore);

    VkImage getCurrentSwapchainImage(uint32_t& imageIndex){return swapchainImages[imageIndex];}
    VkImageView getCurrentSwapchainImageView(uint32_t& imageIndex){return swapchainImageViews[imageIndex];}
    VkImage getDepthImage(uint32_t frameIndex){return depthImages[frameIndex];}
    VkImageView getDepthImageView(uint32_t frameIndex){return depthImageViews[frameIndex];}
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }

    bool compareSwapFormats(const MppSwapChain &swapChain) const {
        return swapChain.depthFormat == depthFormat &&
            swapChain.imageFormat == imageFormat;
    }
    float extentAspectRatio() {
        return static_cast<float>(swapChainExtent.width) /
              static_cast<float>(swapChainExtent.height);
    }

};



} // namespace mpp


