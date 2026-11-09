#include "basic_render_system.hpp"

namespace mpp
{
BasicRenderSystem::BasicRenderSystem(MppDevice &device, const VkFormat& swapChainImageFormat, VkFormat swapChainDepthFormat, VkDescriptorSetLayout descriptorSetLayout): mppDevice{device}
{
    createPipelineLayout(descriptorSetLayout);
    createPipeline(swapChainImageFormat, swapChainDepthFormat);
}

BasicRenderSystem::~BasicRenderSystem()
{
    vkDestroyPipelineLayout(mppDevice.getLogicalDevice(), pipelineLayout, nullptr);
}

void BasicRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
    // GRAPHISC PIPELINE =========================================

    // pipeline stages objects
    // zwykle nie są dynamic(oprócz viewport, scissor), ale za pomocą rzszerzeń mogą być

    /*
    Vulkan supports pipeline types specific to use cases like 
    graphics, compute, and raytracing. As such, setting up a 
    pipeline depends on what we want to achieve. In our case, 
    that's graphics (aka rasterization), so we'll be creating 
    a graphics pipeline.
    */

    // First we create a pipeline layout. 
    // This defines the interface between the pipeline and 
    // our shaders. Pipeline layouts are separate objects as 
    // you can mix and match them for use with other pipelines
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT, //| VK_SHADER_STAGE_FRAGMENT_BIT  TODO:nie wiem czy potrzebne
        .offset = 0,
        .size = sizeof(PushConstants)// max =  128 bajtów
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &globalSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    chk(vkCreatePipelineLayout(mppDevice.getLogicalDevice(), &pipelineLayoutCI, nullptr, &pipelineLayout),"failed to create pipeline layout!");
}

void BasicRenderSystem::createPipeline(VkFormat swapChainImageFormat, VkFormat swapChainDepthFormat)
{
    assert( pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig{};
    MppPipeline::defaultPipelineConfigInfo(pipelineConfig);

    mppPipeline = std::make_unique<MppPipeline>(
        mppDevice,
        pipelineConfig,
        pipelineLayout,
        swapChainImageFormat,
        swapChainDepthFormat
    );
}

void BasicRenderSystem::render(FrameInfo& frameInfo)const
{
    mppPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frameInfo.descriptorSetTex, 0, nullptr);
    
    VkDeviceAddress bufferAddress = frameInfo.bufferAddress;

    for(auto & kv: frameInfo.gameObjects){
        auto& obj = kv.second;
        if(obj.model == nullptr) continue;

        PushConstants push{
            .shaderDataAddress = bufferAddress,
            .objectId = static_cast<uint32_t>(obj.getId())
        };

        
        obj.model->bind(frameInfo.commandBuffer);
    
        //access to data in shader data buffer(to to takie co resizable bar że przez wskaźnik że nie trzeba descriptorów. Znaczy to jest potrzebne do resizable bar, ale te dane przekazywane tutaj to co innego)
        vkCmdPushConstants(frameInfo.commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push);

        obj.model->draw(frameInfo.commandBuffer);
    }

    /*
    These vkCmd* calls (and many others) set the current 
    command buffer state. That means they persist across 
    multiple draw calls inside this command buffer. So if 
    you e.g. wanted to issue a second draw call with the 
    same pipeline but a different descriptor sets you'd 
    only have to call vkCmdBindDescriptorSets with another 
    set, while keeping the rest of the state.
    */
}

std::unique_ptr<MppDescriptorSetLayout> BasicRenderSystem::createTextureSetLayout(MppDevice& device, uint32_t maxTextures) {
    return MppDescriptorSetLayout::Builder(device)
        .addBinding(
            0,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            maxTextures,
            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT// TODO: dodać VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT kiedy będzie się robić łądowanie tekstur w tracie działania programu
        )
        .build();
}

} // namespace mpp
