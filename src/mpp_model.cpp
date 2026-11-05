#include "mpp_model.hpp"

namespace mpp
{

MppModel::MppModel(MppDevice& device, const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, VkDeviceSize indCount): mppDevice{device}, indexCount{indCount}
{
    // tworzenie buffera na vertex i index data
    vBufSize = sizeof(MppModel::Vertex) * vertices.size();
    VkDeviceSize iBufSize{sizeof(uint16_t) * indices.size()};
    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vBufSize + iBufSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };

    // używamy vma do alokacji pamięci buffera na vertex and index data
    VmaAllocationCreateInfo vBufferAllocCI{
        // flagi do resizable BAR !! :D
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    VmaAllocationInfo vBufferAllocInfo{};
    chk(vmaCreateBuffer(mppDevice.getAllocator(), &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation, &vBufferAllocInfo));
    
    /* VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
    Without this we'd have to create a so-called "staging" buffer on the host, 
    copy data to that buffer and then submit a buffer copy from staging to the 
    GPU side buffer using a command buffer. That would require a lot more code.
    */

    //kopiowanie danych do vramu
    memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
    memcpy(((char*)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);
}

void MppModel::bind(VkCommandBuffer commandBuffer)
{
    //3D object data
    VkDeviceSize vOffset{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vBuffer, &vOffset);
    vkCmdBindIndexBuffer(commandBuffer, vBuffer, vBufSize, VK_INDEX_TYPE_UINT16);
}

void MppModel::draw(VkCommandBuffer commandBuffer)
{
    //DRAW :DDDDD
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);//TODO: INSTANCJE OGARNĄĆ
}

} // namespace mpp
