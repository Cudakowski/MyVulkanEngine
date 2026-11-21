#pragma once

#include "mpp_game_object.hpp"
#include "mpp_block_chunk.hpp"

// libs
#include <SDL3/SDL.h>

namespace mpp {

// możliwe że za bardzo rozproszony ten kod, ale na razie jest git
class SceneInputHandler {
public:
    void handleEvent(const SDL_Event& event, float dt, MppGameObject::Map& gameObjects, uint32_t& selectedId, uint32_t& cameraIndex, MppWindow& mppWindow, bool& quit, std::vector<std::unique_ptr<mpp::Block::MppBlockChunk>>& chunks)const;
    float mouseSensitivity{0.01f};
};

} // namespace mpp