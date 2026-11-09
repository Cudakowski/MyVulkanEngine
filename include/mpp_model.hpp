#pragma once

#include "mpp_device.hpp"
#include "mpp_checks.hpp"

// libs

#include <tiny_obj_loader.h>
#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/quaternion.hpp>

// std
#include <memory>

namespace mpp
{


class MppModel 
{
private:
    MppDevice &mppDevice;
    
    VkBuffer vBuffer{VK_NULL_HANDLE};
    VmaAllocation vBufferAllocation{VK_NULL_HANDLE};
    VkDeviceSize indexDataOffset{};

    const uint32_t indexCount{};

public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        bool operator==(const Vertex &other) const{
            return pos == other.pos && normal == other.normal && uv == other.uv;
        }
    };

    struct Builder
    {
        std::vector<Vertex> vertices{};
        std::vector<uint16_t> indices{};

        void loadModel(const std::string &filepath);
    };

    MppModel(MppDevice &device, const MppModel::Builder &builder);
    ~MppModel();

    MppModel(const MppModel &)=delete;
    MppModel &operator=(const MppModel &)=delete;

    static std::unique_ptr<MppModel> createModelFromFile(MppDevice& device, const std::string &filepath);

    void bind(VkCommandBuffer commandBuffer)const;
    void draw(VkCommandBuffer commandBuffer)const;
};


} // namespace mpp





