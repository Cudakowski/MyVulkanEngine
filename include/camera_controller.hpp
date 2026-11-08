#pragma once

#include "mpp_game_object.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <limits>

namespace mpp
{

    
class CameraController 
{
private:

public:
    struct KeyMappings {
        SDL_Scancode moveLeft     = SDL_SCANCODE_A;
        SDL_Scancode moveRight    = SDL_SCANCODE_D;
        SDL_Scancode moveForward  = SDL_SCANCODE_W;
        SDL_Scancode moveBackward = SDL_SCANCODE_S;
        SDL_Scancode moveUp       = SDL_SCANCODE_E;
        SDL_Scancode moveDown     = SDL_SCANCODE_Q;
        SDL_Scancode lookLeft     = SDL_SCANCODE_LEFT;
        SDL_Scancode lookRight    = SDL_SCANCODE_RIGHT;
        SDL_Scancode lookUp       = SDL_SCANCODE_UP;
        SDL_Scancode lookDown     = SDL_SCANCODE_DOWN;
    };

    // Obsługa ciągłego ruchu (WASD / strzałki) wywoływana co klatkę
    void update(float dt, MppGameObject& viewerObject);

    KeyMappings keys{};
    float moveSpeed{3.0f};
    float lookSpeed{1.5f};
};


} // namespace mpp


