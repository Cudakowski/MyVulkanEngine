#pragma once

#include "mpp_device.hpp"
#include "mpp_window.hpp"
#include "mpp_renderer.hpp"
#include "mpp_game_object.hpp"
#include "mpp_texture.hpp"
#include "basic_render_system.hpp"
#include "camera_controller.hpp"
#include "scene_input_handler.hpp"
#include "mpp_camera.hpp"
#include "mpp_descriptors.hpp"

// std
#include <filesystem>


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
    void checkAssets() const;

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


