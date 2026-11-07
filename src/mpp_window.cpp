#include "mpp_window.hpp"

#include <iostream>

namespace mpp
{

//TODO: MOŻE BYĆ POTRZEBNE
// bool SDLCALL MppWindow::eventWatcher(void *userdata, SDL_Event *event) {
//     if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
//         auto *window = reinterpret_cast<MppWindow *>(userdata);
//         window->framebufferResized = true;
//         window->width = event->window.data1;
//         window->height = event->window.data2;
//     }
//     return true;
// }

MppWindow::~MppWindow()
{
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
}

MppWindow::MppWindow( std::string name, int width, int height)
{
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error("failed to initialize SDL");
    }
	if(!SDL_Vulkan_LoadLibrary(NULL))
    {
        throw std::runtime_error("failed to load vulkan library");
    }

    // WINDOW AND SURFACE ==============================

    // tworzenie okna za pomocą SDL'a
    window = SDL_CreateWindow(name.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    //assert(window);// takie tam sprawdzanie czy działa
    if(!window)
    {
        throw std::runtime_error("failed to create window");
    }

    // requestowanie suface'a dla tego okna i sprawdzanie
    // przyjmuje wksźnik do okna, instancje, alokator, i zmienną z 
    // powieżchnią do której zapisuje wynik

    // pobieranie wielkości okna(nie mam pjęcia po jakiego kija pobieramy wielkośc okna kiedy jest to wpisanie zaraz wyżej)
    if(!SDL_GetWindowSizeInPixels(window, &windowSize.x, &windowSize.y))
    {
        throw std::runtime_error("failed to get window size1");
    }

    // // pobieranie właściwości powieżchni
    // if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mppDevice.getPhysicalDevice() , surface, &surfaceCaps) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("failed to get surface capabilities1");
    // }

    

} 




void MppWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR& surface )
{
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface))
    {
        throw std::runtime_error(std::string("Failed to create window surface: ") + SDL_GetError());
    }
}

VkExtent2D MppWindow::getExtent()
{
    if(!SDL_GetWindowSizeInPixels(window, &windowSize.x, &windowSize.y))
    {
        throw std::runtime_error("failed to get window size2");
    }
    //std::cout<<windowSize.x <<' '<<windowSize.y<<" getExtentSize\n";
    return { static_cast<uint32_t>(windowSize.x), static_cast<uint32_t>(windowSize.y) };
}

void MppWindow::setWindowResized(int width, int height)
{
    framebufferResized = true;
    windowSize.x = width;
    windowSize.y = height;
}



} // namespace mpp

