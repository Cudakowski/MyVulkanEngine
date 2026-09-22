#pragma once

#include "mpp_device.hpp"

#define VOLK_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <cassert>
#include <string>

namespace mpp
{

// TODO: podzielić MppWindow na MppWindow i MppSurface
class MppWindow 
{
private:
    VkSurfaceKHR surface{ VK_NULL_HANDLE };// powieżchnia, na której jest wyświetlanie
    glm::ivec2 windowSize{};// wielkość okna
    SDL_Window* window;
    VkSurfaceCapabilitiesKHR surfaceCaps{};

    MppDevice &mppDevice;

public:

    MppWindow(MppDevice &device, std::string name, int width, int height);
    ~MppWindow();

    MppWindow(const MppWindow&) = delete;
    MppWindow& operator=(const MppWindow&) = delete;


    VkSurfaceKHR getSurface(){return surface;}
    VkSurfaceCapabilitiesKHR getSurfaceCaps(){return surfaceCaps;}
    glm::ivec2 getWindowSize(){return windowSize;}
    VkExtent2D getExtent();


};


} // namespace mpp