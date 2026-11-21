#include "block_render_system.hpp"
#include "mpp_block_model.hpp"

namespace mpp::Block
{

BlockRenderSystem::BlockRenderSystem(MppDevice &device, const VkFormat& swapChainImageFormat, VkFormat swapChainDepthFormat, VkDescriptorSetLayout descriptorSetLayout) : mppDevice{device}
{
    createPipelineLayout(descriptorSetLayout);
    createPipeline(swapChainImageFormat, swapChainDepthFormat);
}

BlockRenderSystem::~BlockRenderSystem()
{
    vkDestroyPipelineLayout(mppDevice.getLogicalDevice(), pipelineLayout, nullptr);
}

void BlockRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(BlockPushConstants) // 64  + 8 = 72 
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &globalSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    chk(vkCreatePipelineLayout(mppDevice.getLogicalDevice(), &pipelineLayoutCI, nullptr, &pipelineLayout), "failed to create block pipeline layout!");
}

void BlockRenderSystem::createPipeline(VkFormat swapChainImageFormat, VkFormat swapChainDepthFormat)
{
    assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    MppPipeline::defaultPipelineConfigInfo(pipelineConfig);

    pipelineConfig.bindingDescriptions = MppBlockModel::Vertex::getBindingDescriptions();
    pipelineConfig.attributeDescriptions = MppBlockModel::Vertex::getAttributeDescriptions();

    pipelineConfig.shaderSourcePath = "assets/block_shader.slang";
    pipelineConfig.shaderCachePath = "assets/block_shader.spv";

    mppPipeline = std::make_unique<MppPipeline>(
        mppDevice,
        pipelineConfig,
        pipelineLayout,
        swapChainImageFormat,
        swapChainDepthFormat
    );
}

void BlockRenderSystem::render(FrameInfo& frameInfo, const std::vector<std::unique_ptr<MppBlockChunk>>& chunks) const
{
    mppPipeline->bind(frameInfo.commandBuffer);

    // Możemy użyć dokładnie tego samego descriptorSetTex (Bindless Textures) co w BasicRenderSystem!
    vkCmdBindDescriptorSets(
        frameInfo.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineLayout,
        0,
        1,
        &frameInfo.descriptorSetTex,
        0,
        nullptr
    );

    for (const auto& chunk : chunks)
    {
        if (!chunk || chunk->model.isEmpty()) continue;

        BlockPushConstants push{
            .modelMatrix = chunk->transform.mat4(),
            .shaderDataAddress = frameInfo.bufferAddress
        };

        chunk->model.bind(frameInfo.commandBuffer);
        chunk->model.cleanupRetiredBuffers();

        vkCmdPushConstants(
            frameInfo.commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(BlockPushConstants),
            &push
        );

        chunk->model.draw(frameInfo.commandBuffer);
    }
}

} // namespace mpp::Block