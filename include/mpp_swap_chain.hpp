#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"

#include <vector>

namespace mpp
{

constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2 };

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

    struct ShaderDataBuffer {
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocationInfo{};
        VkBuffer buffer{ VK_NULL_HANDLE };
        VkDeviceAddress deviceAddress{};
    };
    std::array<ShaderDataBuffer, MAX_FRAMES_IN_FLIGHT> shaderDataBuffers;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> commandBuffers;
    struct ShaderData{
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model[3];
        glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f};
        uint32_t selected{1};
    } shaderData{};

    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> fences;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAcquiredSemaphores;
    std::vector<VkSemaphore> renderCompleteSemaphores;

    const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB };
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };

    void createSwapChain();
    void createImageViews();
    void createDepthAttachment();
    void createShaderAndDataBuffers();
    void createSynchronizationObjects();
    void createCommandBuffers();

public:
    MppSwapChain(MppDevice &device, MppWindow &window);

    const VkFormat& getImageFormat(){return imageFormat;}
    VkFormat getDepthFormat(){return depthFormat;}

    ~MppSwapChain()
    {
        for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
        {
            vkDestroyFence(mppDevice.getLogicalDevice(), fences[i], nullptr);
            vkDestroySemaphore(mppDevice.getLogicalDevice(), imageAcquiredSemaphores[i], nullptr);
            vmaDestroyBuffer(mppDevice.getAllocator(), shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
        }

        for (auto i = 0; i < renderCompleteSemaphores.size(); i++) 
        {
            vkDestroySemaphore(mppDevice.getLogicalDevice(), renderCompleteSemaphores[i], nullptr);
        }

        vmaDestroyImage(mppDevice.getAllocator(), depthImage, depthImageAllocation);
        vkDestroyImageView(mppDevice.getLogicalDevice(), depthImageView, nullptr);

        for (auto i = 0; i < swapchainImageViews.size(); i++) 
        {
            vkDestroyImageView(mppDevice.getLogicalDevice(), swapchainImageViews[i], nullptr);
        }

        
    }
};



} // namespace mpp


