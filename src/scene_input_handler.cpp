#include "scene_input_handler.hpp"

namespace mpp
{

void SceneInputHandler::handleEvent(const SDL_Event& event, float dt, MppGameObject::Map& gameObjects, uint32_t& selectedId, uint32_t& cameraIndex, MppWindow& mppWindow, bool& quit, std::vector<std::unique_ptr<mpp::Block::MppBlockChunk>>& chunks)const 
{
    switch (event.type) {

        //Rotate the selected object with mouse drag
        case SDL_EVENT_MOUSE_MOTION:
        {
            if(event.motion.state & SDL_BUTTON_LMASK) //event.button.button == SDL_BUTTON_LEFT
            {
                auto it = gameObjects.find(selectedId);
                if (it != gameObjects.end()) {
                    auto& obj = it->second;
                    // obj.transform.rotation.x -= (float)event.motion.yrel * dt * mouseSensitivity;
                    // obj.transform.rotation.y -= (float)event.motion.xrel * dt * mouseSensitivity;
                    obj.transform.rotation.x -= static_cast<float>(event.motion.yrel) * mouseSensitivity;
                    obj.transform.rotation.y -= static_cast<float>(event.motion.xrel) * mouseSensitivity;

                }
            }
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            if(event.motion.state & SDL_BUTTON_LMASK) //event.button.button == SDL_BUTTON_LEFT
            {
                //chunks[0]->destroyBlock();
            }
            if(event.motion.state & SDL_BUTTON_RMASK) //event.button.button == SDL_BUTTON_LEFT
            {
                
            }
            break;
        }


        // Select active model instance
        case SDL_EVENT_KEY_DOWN:
        {
            if (event.key.key == SDLK_ESCAPE)
            {
                quit = true;
            }
            if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
            {
                selectedId = (selectedId < 2) ? selectedId + 1 : 0;
            }
            if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
            {
                selectedId = (selectedId > 0) ? selectedId - 1 : 2;
            }
            if (event.key.key == SDLK_0 )
            {
                cameraIndex = 0;
                mppWindow.setRelativeMouseMode(false);
            }
            if (event.key.key == SDLK_1 )
            {
                cameraIndex = 1;
                mppWindow.setRelativeMouseMode(true);
            }
            break;
        }

        default:
            break;
    }
}

} // namespace mpp
