#pragma once


#include "mpp_device.hpp"
#include "mpp_model.hpp"
#include "mpp_loader.hpp"
#include "mpp_renderer.hpp"

#include "mpp_checks.hpp"

#define VOLK_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <volk/volk.h>

namespace mpp
{


class MppPipeline 
{
private:
    MppDevice& mppDevice;
    MppLoader& mppLoader;
    MppRenderer& mppRenderer;

    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    VkPipeline pipeline{ VK_NULL_HANDLE };
public:

    MppPipeline(MppDevice& device, MppLoader& loader, MppRenderer& renderer);
};




} // namespace mpp


