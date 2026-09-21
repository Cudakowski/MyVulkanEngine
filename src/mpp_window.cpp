#include "mpp_window.hpp"

#include "mpp_checks.hpp"

namespace mpp
{

MppWindow::MppWindow(VkInstance instance)
{

    // WINDOW AND SURFACE ==============================

    // tworzenie okna za pomocą SDL'a
    SDL_Window* window = SDL_CreateWindow("Ale jak to Vulkan", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    assert(window);// takie tam sprawdzanie czy działa

    // requestowanie suface'a dla tego okna i sprawdzanie
    // przyjmuje wksźnik do okna, instancje, alokator, i zmienną z 
    // powieżchnią do której zapisuje wynik
    chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));

    // pobieranie wielkości okna(nie mam pjęcia po jakiego kija pobieramy wielkośc okna kiedy jest to wpisanie zaraz wyżej)
    chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));

    
} 

} // namespace mpp

