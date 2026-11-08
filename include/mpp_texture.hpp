#pragma once

#include "mpp_device.hpp"

#include "mpp_checks.hpp"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <ktx.h>
#include <ktxvulkan.h>

#include <memory>
#include <string>

namespace mpp
{

class MppTexture 
{
private:
    MppDevice& mppDevice;

    VkDescriptorImageInfo textureDescriptor{};
public:
    VmaAllocation allocation{ VK_NULL_HANDLE };
    VkImage image{ VK_NULL_HANDLE };
    VkImageView view{ VK_NULL_HANDLE };
    VkSampler sampler{ VK_NULL_HANDLE };

    MppTexture(MppDevice& device);
    ~MppTexture();
    
    static std::unique_ptr<MppTexture> createTextureFromFile(MppDevice& mppDevice ,const std::string &file);
    VkDescriptorImageInfo getDescriptorInfo(){return textureDescriptor;}
};




} // namespace mpp


