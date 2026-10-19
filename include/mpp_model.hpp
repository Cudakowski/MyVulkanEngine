#pragma once

#include "mpp_device.hpp"

#include "mpp_checks.hpp"

#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/quaternion.hpp>

namespace mpp
{


class MppModel 
{
private:
    MppDevice &mppDevice;
    
    VkBuffer vBuffer{VK_NULL_HANDLE};
    VmaAllocation vBufferAllocation{VK_NULL_HANDLE};

public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    MppModel(MppDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);
    ~MppModel()
    {
        vmaDestroyBuffer(mppDevice.getAllocator(), vBuffer, vBufferAllocation);
    }
};


} // namespace mpp





