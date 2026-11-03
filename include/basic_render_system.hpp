#pragma once

#include "mpp_device.hpp"
#include "mpp_pipeline.hpp"
#include "mpp_renderer.hpp"
#include "mpp_loader.hpp"
#include "mpp_frame_info.hpp"

#include <memory>

namespace mpp
{
  
class BasicRenderSystem 
{
private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline();

    MppDevice &mppDevice;
    MppRenderer &mppRenderer;
    MppLoader &mppLoader;
    std::unique_ptr<MppPipeline> mppPipeline;
    VkPipelineLayout pipelineLayout;


public:
    BasicRenderSystem(MppDevice &device, MppRenderer &renderer, MppLoader &loader);
    ~BasicRenderSystem();

    BasicRenderSystem(const BasicRenderSystem &) = delete;
    BasicRenderSystem &operator=(const BasicRenderSystem &) = delete;

    void render(FrameInfo& frameInfo);


};



} // namespace mpp