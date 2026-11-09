#pragma once

#include "mpp_device.hpp"
#include "mpp_pipeline.hpp"
#include "mpp_descriptors.hpp"
#include "mpp_frame_info.hpp"

// std
#include <memory>

namespace mpp
{
  
class BasicRenderSystem 
{
private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkFormat swapChainImageFormat, VkFormat swapChainDepthFormat);

    MppDevice &mppDevice;


    std::unique_ptr<MppPipeline> mppPipeline;
    VkPipelineLayout pipelineLayout;


public:
    BasicRenderSystem(MppDevice &device, const VkFormat& swapChainImageFormat, VkFormat swapChainDepthFormat, VkDescriptorSetLayout descriptorSetLayout);
    ~BasicRenderSystem();

    BasicRenderSystem(const BasicRenderSystem &) = delete;
    BasicRenderSystem &operator=(const BasicRenderSystem &) = delete;

    void render(FrameInfo& frameInfo)const;

    static std::unique_ptr<MppDescriptorSetLayout> createTextureSetLayout(MppDevice& device, uint32_t maxTextures);
};



} // namespace mpp