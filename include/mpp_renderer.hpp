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

    uint32_t currentImageIndex{0};
    uint32_t frameIndex{0};
    bool updateSwapchain{false};//to delete
    bool isFrameStarted{false};

    void createCommandBuffers();
    void createShaderDataBuffers();
    void freeCommandBuffers();
    void recreateSwapChain();
    void createSynchronizationObjects();

    struct ShaderDataBuffer {
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocationInfo{};
        VkBuffer buffer{ VK_NULL_HANDLE };
        VkDeviceAddress deviceAddress{};
    };
    std::array<ShaderDataBuffer, MppSwapChain::MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
    std::array<VkCommandBuffer, MppSwapChain::MAX_FRAMES_IN_FLIGHT> commandBuffers;
    
    std::array<VkFence, MppSwapChain::MAX_FRAMES_IN_FLIGHT> fences;
    std::array<VkSemaphore, MppSwapChain::MAX_FRAMES_IN_FLIGHT> imageAcquiredSemaphores;

    ShaderDataBuffer& getCurrentShaderDataBuffer(){return shaderDataBuffers[frameIndex];}// nie wiem czy potrzebne
    bool hasStencilComponent(VkFormat format);
    
public:

    MppRenderer(MppDevice& device, MppWindow& window);
    ~MppRenderer();

    MppRenderer(const MppRenderer&) = delete;
    MppRenderer& operator=(const MppRenderer&) = delete;

    const VkFormat& getSwapChainImageFormat(){return mppSwapChain->getImageFormat();}//to sie używa przy tworzeniu pipeline'u
    VkFormat getSwapChainDepthFormat(){return mppSwapChain->getDepthFormat();}//to sie używa przy tworzeniu pipeline'u
    VkDeviceAddress& getCurrentShaderDataBufferDeviceAddress(){return getCurrentShaderDataBuffer().deviceAddress;}//to sie używa w systemach renderowania
    //int getFrameIndex(){return ;}


    VkCommandBuffer beginFrame();
    void endFrame(VkCommandBuffer commandBuffer);
    void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
    void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

    
    bool isFrameInProgress() const { return isFrameStarted; }
    //float getAspectRatio() const {return mppSwapChain->extentAspectRatio();}//a to może być nawet kiedyś potrzebne xd

    void updateShaderData(ShaderData& shaderData);//a co to xd
    void conditionalRecreateSwapChain();//to delete
    void requestRecreateSwapChain(){updateSwapchain = true;};//to delete

    VkCommandBuffer getCurrentCommandBuffer() const {
        assert(isFrameStarted && "Cannot get buffer when frame not in progress");
        return commandBuffers[frameIndex];
    }
    int getFrameIndex() const {//ta funkcja chyba nie jest w cale potrzebna
        assert(isFrameStarted && "Cannot get frame index when frame not in progress");
        return frameIndex;
    }
    float getAspectRatio() const {return mppSwapChain->extentAspectRatio();}

};



} // namespace mpp


