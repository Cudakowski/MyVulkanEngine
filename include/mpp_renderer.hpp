#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_swap_chain.hpp"

#include <vulkan/vulkan.h>
#include <volk/volk.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "mpp_checks.hpp"

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

    uint32_t imageIndex{0};
    void recreateSwapChain();



public:

    MppRenderer(MppDevice& device, MppWindow& window);
    ~MppRenderer();

    const VkFormat& getSwapChainImageFormat(){return mppSwapChain->getImageFormat();}
    VkFormat getSwapChainDepthFormat(){return mppSwapChain->getDepthFormat();}
    VkDeviceAddress& getCurrentShaderDataBufferDeviceAddress(){return mppSwapChain->getCurrentShaderDataBuffer().deviceAddress;}
    //int getFrameIndex(){return ;}

    MppRenderer(const MppRenderer&) = delete;
    MppRenderer& operator=(const MppRenderer&) = delete;

    VkCommandBuffer beginFrame();
    void endFrame(VkCommandBuffer commandBuffer);
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void updateShaderData(ShaderData& shaderData);

};



} // namespace mpp


