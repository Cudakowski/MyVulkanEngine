#pragma once

#include "mpp_game_object.hpp"
#include <SDL3/SDL.h>

namespace mpp {

// możliwe że za bardzo rozproszony ten kod, ale na razie jest git
class SceneInputHandler {
public:
    void handleEvent(const SDL_Event& event, float dt, MppGameObject::Map& gameObjects, uint32_t& selectedId);
};

} // namespace mpp