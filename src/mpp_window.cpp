#include "mpp_window.hpp"


namespace mpp
{

MppWindow::MppWindow(MppDevice &device, std::string name, int width, int height): mppDevice{device}
{

    // WINDOW AND SURFACE ==============================

    // tworzenie okna za pomocą SDL'a
    window = SDL_CreateWindow("Ale jak to Vulkan", 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    assert(window);// takie tam sprawdzanie czy działa

    // requestowanie suface'a dla tego okna i sprawdzanie
    // przyjmuje wksźnik do okna, instancje, alokator, i zmienną z 
    // powieżchnią do której zapisuje wynik
    chk(SDL_Vulkan_CreateSurface(window, mppDevice.getInstance(), nullptr, &surface));

    // pobieranie wielkości okna(nie mam pjęcia po jakiego kija pobieramy wielkośc okna kiedy jest to wpisanie zaraz wyżej)
    chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));

    // pobieranie właściwości powieżchni
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mppDevice.getPhysicalDevice() , surface, &surfaceCaps));

} 

VkExtent2D MppWindow::getExtent()
{
    // pobieramy extent
    VkExtent2D swapchainExtent{ surfaceCaps.currentExtent };
    if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
        swapchainExtent = { .width = static_cast<uint32_t>(windowSize.x), .height=static_cast<uint32_t>(windowSize.y)};
    }
    return swapchainExtent;
}

} // namespace mpp

