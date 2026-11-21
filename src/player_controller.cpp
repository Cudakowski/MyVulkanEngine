#include "player_controller.hpp"

namespace mpp
{

void PlayerController::update(float dt, MppGameObject& playerObject, glm::vec3& playerSpeed) const
{
    const bool* keyboardState = SDL_GetKeyboardState(nullptr);
    float x;
    float y;
    //auto mouseState = SDL_GetRelativeMouseState(&x,&y);
    SDL_GetRelativeMouseState(&x,&y);

    playerObject.transform.rotation.x -= y * mouseSensitivity;
    playerObject.transform.rotation.y += x * mouseSensitivity;

    // limit pitch values between about +/- 85ish degrees
    playerObject.transform.rotation.x = glm::clamp(playerObject.transform.rotation.x, -1.5f, 1.5f);
    playerObject.transform.rotation.y = glm::mod(playerObject.transform.rotation.y, glm::two_pi<float>());

    float yaw = playerObject.transform.rotation.y;
    const glm::vec2 forwardDir{sin(yaw), cos(yaw)};
    const glm::vec2 rightDir{forwardDir.y,  -forwardDir.x};
    // const glm::vec3 upDir{0.0f, -1.0f, 0.0f};

    glm::vec2 moveDir{0.0f};
    if (keyboardState[keys.moveForward])   moveDir += forwardDir;
    if (keyboardState[keys.moveBackward])  moveDir -= forwardDir;
    if (keyboardState[keys.moveRight])     moveDir += rightDir;
    if (keyboardState[keys.moveLeft])      moveDir -= rightDir;
    // if (keyboardState[keys.moveUp])        moveDir += upDir;
    // if (keyboardState[keys.moveDown])      moveDir -= upDir;
    

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        moveDir = glm::normalize(moveDir);
        playerSpeed.x = moveSpeed * moveDir.x;
        playerSpeed.z = moveSpeed * moveDir.y;
    }
    else
    {
        playerSpeed.x = 0.0f;
        playerSpeed.z = 0.0f;
    }

    if (keyboardState[keys.jump] && playerSpeed.y == 0.0f) playerSpeed.y += -6.f;
}

} // namespace mpp
