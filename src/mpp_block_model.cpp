#include "mpp_block_model.hpp"
#include <cstring>

namespace mpp::Block
{

MppBlockModel::MppBlockModel(MppDevice &device) : mppDevice{device} {}

MppBlockModel::~MppBlockModel()
{
    if (vBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(mppDevice.getAllocator(), vBuffer, vBufferAllocation);
    }
    for (auto& rb : retiredBuffers) {
        vmaDestroyBuffer(mppDevice.getAllocator(), rb.buffer, rb.allocation);
    }
}

void MppBlockModel::destroyBuffer()
{
    if (vBuffer != VK_NULL_HANDLE)
    {
        // vmaDestroyBuffer(mppDevice.getAllocator(), vBuffer, vBufferAllocation);
        retiredBuffers.push_back({vBuffer, vBufferAllocation, 3});
        vBuffer = VK_NULL_HANDLE;
        vBufferAllocation = VK_NULL_HANDLE;
        indexCount = 0;
    }
}


void MppBlockModel::cleanupRetiredBuffers()
{
    for (auto it = retiredBuffers.begin(); it != retiredBuffers.end(); )
    {
        if (it->framesRemaining == 0)
        {
            vmaDestroyBuffer(mppDevice.getAllocator(), it->buffer, it->allocation);
            it = retiredBuffers.erase(it);
        }
        else
        {
            --it->framesRemaining;
            ++it;
        }
    }
}

void MppBlockModel::updateMesh(const Builder &builder)
{
    destroyBuffer();

    if (builder.vertices.empty() || builder.indices.empty())
    {
        return; 
    }

    indexCount = static_cast<uint32_t>(builder.indices.size());

    VkDeviceSize vBufSize = sizeof(Vertex) * builder.vertices.size();
    indexDataOffset = (vBufSize + 3) & ~static_cast<VkDeviceSize>(3);
    VkDeviceSize iBufSize = sizeof(uint32_t) * builder.indices.size();// 32 a nie 16

    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = indexDataOffset + iBufSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };

    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VmaAllocationInfo allocInfo{};
    chk(vmaCreateBuffer(mppDevice.getAllocator(), &bufferCI, &allocCI, &vBuffer, &vBufferAllocation, &allocInfo));

    char* mappedPtr = static_cast<char*>(allocInfo.pMappedData);
    memcpy(mappedPtr, builder.vertices.data(), vBufSize);
    memcpy(mappedPtr + indexDataOffset, builder.indices.data(), iBufSize);

    vmaFlushAllocation(mppDevice.getAllocator(), vBufferAllocation, 0, VK_WHOLE_SIZE);
}

void MppBlockModel::bind(VkCommandBuffer commandBuffer) const
{
    if (indexCount == 0) return;

    VkDeviceSize vOffset{0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vBuffer, &vOffset);
    vkCmdBindIndexBuffer(commandBuffer, vBuffer, indexDataOffset, VK_INDEX_TYPE_UINT32); // uwaga : uint32
}

void MppBlockModel::draw(VkCommandBuffer commandBuffer) const
{
    if (indexCount == 0) return;

    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
}

std::vector<VkVertexInputBindingDescription> MppBlockModel::Vertex::getBindingDescriptions()
{
    return {
        {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };
}

std::vector<VkVertexInputAttributeDescription> MppBlockModel::Vertex::getAttributeDescriptions()
{
    return {
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos) },
        { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal) },
        { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv) },
        { .location = 3, .binding = 0, .format = VK_FORMAT_R32_UINT,       .offset = offsetof(Vertex, texID) }
    };
}

} // namespace mpp::Block