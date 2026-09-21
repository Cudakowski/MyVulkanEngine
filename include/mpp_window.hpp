#pragma once


#define VOLK_IMPLEMENTATION
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <cassert>

namespace mpp
{


class MppWindow 
{
private:
    VkSurfaceKHR surface{ VK_NULL_HANDLE };// powieżchnia, na której jest wyświetlanie
    glm::ivec2 windowSize{};// wielkość okna

public:

    MppWindow(VkInstance instance);

};


} // namespace mpp