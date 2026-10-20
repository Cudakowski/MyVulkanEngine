#pragma once

#include "mpp_game_object.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mpp
{

    
struct ShaderData{
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model[3];
    glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f};
    uint32_t selected{1};
};

struct FrameInfo {
    //int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    //SspCamera &camera;
    VkDescriptorSet descriptorSetTex;
    MppGameObject::Map &gameObjects;
};


} // namespace mpp



