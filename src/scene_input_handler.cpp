#include "scene_input_handler.hpp"

namespace mpp
{

void SceneInputHandler::handleEvent(const SDL_Event& event, float dt, MppGameObject::Map& gameObjects, uint32_t& selectedId) 
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
                    obj.transform.rotation.x -= (float)event.motion.yrel * dt;
                    obj.transform.rotation.y -= (float)event.motion.xrel * dt;
                }
            }
            break;
        }


        // Select active model instance
        case SDL_EVENT_KEY_DOWN:
        {
            if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
            {
                selectedId = (selectedId < 2) ? selectedId + 1 : 0;
            }
            if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
            {
                selectedId = (selectedId > 0) ? selectedId - 1 : 2;
            }
            break;
        }

        default:
            break;
    }
}

} // namespace mpp
