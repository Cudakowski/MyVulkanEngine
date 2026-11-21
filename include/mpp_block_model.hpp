#pragma once

#include "mpp_device.hpp"
#include "mpp_checks.hpp"

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace mpp::Block
{

struct RetiredBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    uint32_t framesRemaining; 
};


class MppBlockModel
{
private:
    MppDevice &mppDevice;
    
    VkBuffer vBuffer{VK_NULL_HANDLE};
    VmaAllocation vBufferAllocation{VK_NULL_HANDLE};
    VkDeviceSize indexDataOffset{};

    uint32_t indexCount{0};

    void destroyBuffer();

    std::vector<RetiredBuffer> retiredBuffers;

public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
        uint32_t texID;

        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        // bool operator==(const Vertex &other) const{
        //     return pos == other.pos && normal == other.normal && uv == other.uv;
        // }
    };

    struct Builder
    {
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
    };

    MppBlockModel(MppDevice &device);
    ~MppBlockModel();


    MppBlockModel(const MppBlockModel&) = delete;
    MppBlockModel& operator=(const MppBlockModel&) = delete;

    

    // static std::unique_ptr<MppModel> createModelFromFile(MppDevice& device, const std::string &filepath);

    void updateMesh(const Builder &builder);
    void cleanupRetiredBuffers();

    void bind(VkCommandBuffer commandBuffer) const;
    void draw(VkCommandBuffer commandBuffer) const;
    bool isEmpty() const { return indexCount == 0; }
};

} // namespace mpp
