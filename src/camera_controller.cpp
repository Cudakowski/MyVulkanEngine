#include "camera_controller.hpp"
#include <glm/gtc/constants.hpp>
#include <limits>

namespace mpp
{
    
void CameraController::update(float dt, MppGameObject& viewerObject) {
    const bool* keyboardState = SDL_GetKeyboardState(nullptr);

    glm::vec3 rotate{0.0f};
    if (keyboardState[keys.lookRight]) rotate.y += 1.0f;
    if (keyboardState[keys.lookLeft])  rotate.y -= 1.0f;
    if (keyboardState[keys.lookUp])    rotate.x += 1.0f;
    if (keyboardState[keys.lookDown])  rotate.x -= 1.0f;

    if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon()) 
    {
        viewerObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
    }

    // limit pitch values between about +/- 85ish degrees
    viewerObject.transform.rotation.x = glm::clamp(viewerObject.transform.rotation.x, -1.5f, 1.5f);
    viewerObject.transform.rotation.y = glm::mod(viewerObject.transform.rotation.y, glm::two_pi<float>());

    float yaw = viewerObject.transform.rotation.y;
    const glm::vec3 forwardDir{sin(yaw), 0.0f, cos(yaw)};
    const glm::vec3 rightDir{forwardDir.z, 0.0f, -forwardDir.x};
    const glm::vec3 upDir{0.0f, -1.0f, 0.0f};

    glm::vec3 moveDir{0.0f};
    if (keyboardState[keys.moveForward])   moveDir += forwardDir;
    if (keyboardState[keys.moveBackward])  moveDir -= forwardDir;
    if (keyboardState[keys.moveRight])     moveDir += rightDir;
    if (keyboardState[keys.moveLeft])      moveDir -= rightDir;
    if (keyboardState[keys.moveUp])        moveDir += upDir;
    if (keyboardState[keys.moveDown])      moveDir -= upDir;

    if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) {
        viewerObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
    }
}

} // namespace mpp
