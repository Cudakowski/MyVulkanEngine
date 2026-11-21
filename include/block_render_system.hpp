#pragma once

#include "mpp_device.hpp"
#include "mpp_pipeline.hpp"
#include "mpp_descriptors.hpp"
#include "mpp_frame_info.hpp"
#include "mpp_block_chunk.hpp" 

// std
#include <memory>
#include <vector>

namespace mpp::Block
{

struct BlockPushConstants {
    glm::mat4 modelMatrix{1.0f};
    VkDeviceAddress shaderDataAddress{0}; // Zachowujemy dostęp do danych globalnych (np. macierzy kamery/widoku)
};

class BlockRenderSystem
{
private:
    void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
    void createPipeline(VkFormat swapChainImageFormat, VkFormat swapChainDepthFormat);

    MppDevice &mppDevice;

    std::unique_ptr<MppPipeline> mppPipeline;
    VkPipelineLayout pipelineLayout{VK_NULL_HANDLE};

public:
    BlockRenderSystem(MppDevice &device, const VkFormat& swapChainImageFormat, VkFormat swapChainDepthFormat, VkDescriptorSetLayout descriptorSetLayout);
    ~BlockRenderSystem();

    BlockRenderSystem(const BlockRenderSystem &) = delete;
    BlockRenderSystem &operator=(const BlockRenderSystem &) = delete;

    void render(FrameInfo& frameInfo, const std::vector<std::unique_ptr<MppBlockChunk>>& chunks) const;
};

} // namespace mpp::Block