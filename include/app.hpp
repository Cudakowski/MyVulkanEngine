#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_pipeline.hpp"
#include "mpp_renderer.hpp"
#include "mpp_loader.hpp"
#include "mpp_game_object.hpp"
#include "mpp_texture.hpp"

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
    bool updateSwapchain{ false };
    
class App
{
private:

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
    MppLoader mppLoader{mppDevice};

    MppGameObject::Map gameOjects;

    void loadGameObjects();

public:
    App();
    ~App();

    void run();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;
};



} // namespace mpp


