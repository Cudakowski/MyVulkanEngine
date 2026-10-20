#pragma once

#include "mpp_device.hpp"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace mpp
{

class MppTexture 
{
private:
    MppDevice& mppDevice;
public:
    VmaAllocation allocation{ VK_NULL_HANDLE };
    VkImage image{ VK_NULL_HANDLE };
    VkImageView view{ VK_NULL_HANDLE };
    VkSampler sampler{ VK_NULL_HANDLE };

    MppTexture(MppDevice& device): mppDevice{device} {}

    ~MppTexture()
    {
        vkDestroyImageView(mppDevice.getLogicalDevice(), view, nullptr);
        vkDestroySampler(mppDevice.getLogicalDevice(), sampler, nullptr);
        vmaDestroyImage(mppDevice.getAllocator(), image, allocation);
    }
    
};




} // namespace mpp


