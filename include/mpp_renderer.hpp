#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_swap_chain.hpp"

#include <vulkan/vulkan.h>

#include <memory>

namespace mpp
{

    
class MppRenderer 
{
private:

    MppDevice& mppDevice;
    MppWindow& mppWindow;

    std::unique_ptr<MppSwapChain> mppSwapChain;
    std::vector<VkCommandBuffer> commandBuffers;

public:

    MppRenderer(MppDevice& device, MppWindow& window);
    ~MppRenderer();

    const VkFormat& getSwapChainImageFormat(){return mppSwapChain->getImageFormat();}
    VkFormat getSwapChainDepthFormat(){return mppSwapChain->getDepthFormat();}

    MppRenderer(const MppRenderer&) = delete;
    MppRenderer& operator=(const MppRenderer&) = delete;

};



} // namespace mpp


