#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_pipeline.hpp"
#include "mpp_renderer.hpp"
#include "mpp_loader.hpp"
#include "mpp_game_object.hpp"

#define VOLK_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include <ktx.h>
#include <ktxvulkan.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <filesystem>
//#include <cassert>

#include "mpp_checks.hpp"

namespace mpp
{
    constexpr uint32_t maxFramesInFlight{ 2 };
    bool updateSwapchain{ false };
    
class App
{
private:

    
    
    struct ShaderDataBuffer {
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VmaAllocationInfo allocationInfo{};
        VkBuffer buffer{ VK_NULL_HANDLE };
        VkDeviceAddress deviceAddress{};
    };
    std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
    std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers;
    struct ShaderData{
        glm::mat4 projection;
        glm::mat4 view;
        glm::mat4 model[3];
        glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f};
        uint32_t selected{1};
    } shaderData{};
    std::array<VkFence, maxFramesInFlight> fences;
    std::array<VkSemaphore, maxFramesInFlight> imageAcquiredSemaphores;
    std::vector<VkSemaphore> renderCompleteSemaphores;
    VkCommandPool commandPool{ VK_NULL_HANDLE };
    struct Texture {
        VmaAllocation allocation{ VK_NULL_HANDLE };
        VkImage image{ VK_NULL_HANDLE };
        VkImageView view{ VK_NULL_HANDLE };
        VkSampler sampler{ VK_NULL_HANDLE };
    };
    std::array<Texture, 3> textures{};
    VkDescriptorSetLayout descriptorSetLayoutTex{ VK_NULL_HANDLE};
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSetTex{VK_NULL_HANDLE};
    Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
    VkPipeline pipeline{ VK_NULL_HANDLE };
    uint32_t frameIndex{0};
    
    uint32_t imageIndex{0};
    glm::vec3 camPos{0.0f, 0.0f, -6.0f};
    glm::vec3 objectRotations[3]{};


    static inline void chkSwapchain(VkResult result) {
        if (result < VK_SUCCESS) {
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                updateSwapchain = true;
                return;
            }
            std::cerr << "Vulkan call returned an error (" << result << ")\n";
            exit(result);
        }
    }


    MppDevice mppDevice{};
    MppWindow mppWindow{mppDevice, "Ale jak to vulkan", 1280, 720};
    MppRenderer mppRenderer{mppDevice,mppWindow};
    MppLoader mppLoader{};

    MppGameObject::Map gameOjects;

    void loadGameObjects();

public:
    App(/* args */);
    ~App();

    void run();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;
};



} // namespace mpp


