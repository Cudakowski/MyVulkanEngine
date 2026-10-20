#include "basic_render_system.hpp"

namespace mpp
{
BasicRenderSystem::BasicRenderSystem(MppDevice &device, MppRenderer &renderer, MppLoader &loader): mppDevice{device}, mppRenderer{renderer}, mppLoader{loader}
{
    createPipeline();
}

BasicRenderSystem::~BasicRenderSystem()
{

}

void BasicRenderSystem::createPipeline()
{
    mppPipeline = std::make_unique<MppPipeline>(
        mppDevice,
        mppLoader,
        mppRenderer
    );
}

void BasicRenderSystem::render(FrameInfo& frameInfo)
{
    mppPipeline->bind(frameInfo.commandBuffer);

    vkCmdBindDescriptorSets(frameInfo.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mppPipeline->getPipelineLayout(), 0, 1, &frameInfo.descriptorSetTex, 0, nullptr);
    
    for(auto & kv: frameInfo.gameObjects){
        auto& obj = kv.second;
        if(obj.model == nullptr) continue;
        // SimplePushConstantData push{};
        // push.modelMatrix = obj.transform.mat4();
        // push.normalMatrix = obj.transform.normalMatrix();

        // vkCmdPushConstants(
        // frameInfo.commandBuffer,
        // pipelineLayout,
        // VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        // 0,
        // sizeof(SimplePushConstantData),
        // &push);

        
        obj.model->bind(frameInfo.commandBuffer);
        
    
        //access to data in shader data buffer(to to takie co resizable bar że przez wskaźnik że nie trzeba descriptorów)
        vkCmdPushConstants(frameInfo.commandBuffer, mppPipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &mppRenderer.getCurrentShaderDataBufferDeviceAddress());

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


} // namespace mpp
