#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_pipeline.hpp"
#include "mpp_renderer.hpp"
#include "mpp_game_object.hpp"
#include "mpp_texture.hpp"
#include "mpp_frame_info.hpp"
#include "basic_render_system.hpp"
#include "camera_controller.hpp"
#include "scene_input_handler.hpp"
#include "mpp_camera.hpp"
#include "mpp_descriptors.hpp"

#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
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
    
class App
{
private:
    MppWindow mppWindow{ "Ale jak to vulkan", 1280, 720};
    MppDevice mppDevice{mppWindow};
    MppRenderer mppRenderer{mppDevice,mppWindow};


    std::unique_ptr<MppDescriptorPool> globalPool{};

    VkDescriptorSet textureDescriptorSet{VK_NULL_HANDLE};
    std::vector<std::unique_ptr<MppTexture>> textures{};
    std::vector<VkDescriptorImageInfo> textureDescriptors{};

    MppGameObject::Map gameObjects;
    std::unique_ptr<BasicRenderSystem> basicRenderSystem{};

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


