#pragma once

#include "mpp_game_object.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mpp
{

struct PushConstants {
    VkDeviceAddress shaderDataAddress;
    uint32_t objectId;
};

struct MeshObjectData {
    glm::mat4 modelMatrix{1.f};
    glm::mat4 normalMatrix{1.f};
};
    
struct ShaderData{
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f};
    glm::vec4 cameraPos;
    uint32_t selected{1};
    uint32_t _padding[3];
    MeshObjectData objectData[128];
};

struct FrameInfo {
    //int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    //SspCamera &camera;
    VkDescriptorSet descriptorSetTex;
    MppGameObject::Map &gameObjects;
    VkDeviceAddress bufferAddress;
};


} // namespace mpp



