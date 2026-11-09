#pragma once

#include "mpp_checks.hpp"

// libs
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <cassert>
#include <string>
#include <iostream>

namespace mpp
{


class MppWindow 
{
private:
    //VkSurfaceKHR surface{ VK_NULL_HANDLE };// powieżchnia, na której jest wyświetlanie
    glm::ivec2 windowSize{};// wielkość okna
    SDL_Window* window;
    //VkSurfaceCapabilitiesKHR surfaceCaps{};


    bool framebufferResized{false};

public:

    MppWindow( std::string name, int width, int height);
    ~MppWindow();

    MppWindow(const MppWindow&) = delete;
    MppWindow& operator=(const MppWindow&) = delete;


    glm::ivec2 getWindowSize()const{return windowSize;}

    /**
     * @brief Get the Window Extent
     * 
     * Function returns window extent in pixels. Not the actual surface extent(this one is acquired inside swapchain creation)
     * 
     * @return VkExtent2D 
     */
    VkExtent2D getExtent();
    bool wasWindowResized()const{return framebufferResized;}
    void resetWindowResizedFlag(){ framebufferResized=false;}
    void createWindowSurface(VkInstance instance, VkSurfaceKHR& surface )const;
    void setWindowResized(int width, int height);
    

};


} // namespace mpp