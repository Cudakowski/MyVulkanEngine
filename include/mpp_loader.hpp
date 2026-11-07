#pragma once

#include "mpp_model.hpp"
#include "mpp_texture.hpp"

#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <ktx.h>
#include <ktxvulkan.h>
#include <tiny_obj_loader.h>
#include <vma/vk_mem_alloc.h>
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include "mpp_checks.hpp"

#include <vector>
#include <memory>
#include <string>
#include <array>

namespace mpp
{

class MppLoader
{
private:
    uint32_t textureCount{0}; 

    VkDescriptorSetLayout descriptorSetLayoutTex{ VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSetTex{VK_NULL_HANDLE};
    std::vector<VkDescriptorImageInfo> textureDescriptors{};
    
    Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
    VkShaderModule shaderModule{};

public:
    MppLoader(MppDevice& device);
    ~MppLoader();

    MppDevice& mppDevice;

    std::unique_ptr<MppTexture> createTextureFromFile(const std::string &file);
    void initializeTextures();
    void loadShaders();

    VkDescriptorSetLayout& getDescriptorSetLayoutTex(){ return descriptorSetLayoutTex;};
    VkDescriptorSet getDescriptorSetTex(){return descriptorSetTex;}
    VkShaderModule getShaderModule(){return shaderModule;};

    //std::vector<VkDescriptorImageInfo>& getTextureDescriptors(){return textureDescriptors;}

    

};




} // namespace mpp


