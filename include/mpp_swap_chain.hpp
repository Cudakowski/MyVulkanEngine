#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"

#include <vector>

namespace mpp
{


class MppSwapChain 
{
private:
    MppDevice &mppDevice;
    MppWindow &mppWindow;
    VkExtent2D swapchainExtent;

    VkSwapchainKHR swapchain{ VK_NULL_HANDLE };// swapchain
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkImage depthImage;
    VmaAllocation depthImageAllocation;
    VkImageView depthImageView;

    const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB };

    void createSwapChain();
    void createImageViews();
    void createDepthAttachment();

public:
    MppSwapChain(MppDevice &device, MppWindow &window);
};



} // namespace mpp


