#include "mpp_renderer.hpp"

namespace mpp
{


MppRenderer::MppRenderer(MppDevice& device, MppWindow& window): mppDevice{device}, mppWindow{window}
{
    mppSwapChain = std::make_unique<MppSwapChain>(mppDevice, mppWindow);
}

MppRenderer::~MppRenderer()
{
    
}

VkCommandBuffer MppRenderer::beginFrame()
{
    auto result = mppSwapChain->acquireNextImage(imageIndex);
    if (result < VK_SUCCESS) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return nullptr;
        }
        std::cerr << "Vulkan call returned an error (" << result << ")\n";
        exit(result);
    }

    
    // Record command buffer ********************************
    /*
    You might be tempted to pre-record command buffers 
    and reuse them until something changes that would 
    require re-recording. This makes things unnecessarily 
    complicated however, as you'd have to implement update 
    logic that works with CPU/GPU parallelism. And since 
    recording command buffers is relatively fast and can be 
    offloaded to other CPU threads if needed, recording them 
    every frame is perfectly fine.
    */

    /*
    Commands that are recorded into a command buffer start with 
    vkCmd. They are not directly executed, but only when the 
    command buffer is submitted to a queue (GPU timeline). 
    This also explains why these commands don't return a result. 
    A common mistake for beginners is to mix those commands with 
    commands that are instantly executed on the CPU timeline. 
    It's important to remember that these two different timelines 
    exist.
    */

    //funkcje zaczynające się na vkCmd są nagrywane a nie wykonywane!!!!!
    // nagrywać funkcje można tylko do buffera, który nie jest obecnie wykonywany, a zapewnił to fence
    
    //resetowanie buffera
    auto cb = mppSwapChain->getCurrentCommandBuffer();
    chk(vkResetCommandBuffer(cb,0));

    //rozpoczęcie nagrywania
    VkCommandBufferBeginInfo cbBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    chk(vkBeginCommandBuffer(cb, &cbBI));

    return cb;

    
}

void MppRenderer::beginSwapChainRenderPass(VkCommandBuffer cb)
{
    //zlecamy zmienienie layoutu swpachain image oraz depth image na ten optymalny zależny od urządzenia
    std::array<VkImageMemoryBarrier2, 2> outputBarriers{
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, //czekaj an tego stage'a
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = mppSwapChain->getCurrentSwapchainImage(imageIndex),
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
        },
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, //czekaj an tego stage'a
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = mppSwapChain->getDepthImage(),
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 }
        },
    };
    VkDependencyInfo barrierDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers = outputBarriers.data()
    };
    vkCmdPipelineBarrier2(cb, &barrierDependencyInfo);
    // Not only do image memory barriers transition layouts, 
    // they also make sure that this happens at the right 
    // pipeline stage, enforcing ordering inside a command buffer.

    /*
    Available and visible might sound like the same thing, 
    but they aren't. That's due to how CPU/GPUs work and how 
    they interact with caches. Available means data is ready 
    for future memory operations (e.g. cache flushes). Visible 
    means that data is actually visible to reads from the 
    consuming stages.
    */

    /*
    The VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL layout is a core 
    feature of Vulkan 1.3 that combines all types of attachment 
    layouts into a single one. This simplifies image barriers.
    */

    //dynamic rendering :D
    VkRenderingAttachmentInfo colorAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = mppSwapChain->getCurrentSwapchainImageView(imageIndex),
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue{.color{0.0f, 0.0f, 0.2f, 1.0f}}
    };
    VkRenderingAttachmentInfo depthAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = mppSwapChain->getDepthImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue{.depthStencil = {1.0f, 0}}
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea{.extent{.width = static_cast<uint32_t>(mppWindow.getWindowSize().x), .height = static_cast<uint32_t>(mppWindow.getWindowSize().y)}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo
    };
    vkCmdBeginRendering(cb, &renderingInfo); // :DDDD

    //wiewport(widzialne) i scissor(to co nie wycinać)
    // to te dynamiczne rzeczy(nie trzeba ponownie tworzyć 
    // pipeline'u przy każdym zmienieniu wielkości onka :D)
    VkViewport vp{
        .width = static_cast<float>(mppWindow.getWindowSize().x),
        .height = static_cast<float>(mppWindow.getWindowSize().y),
        .minDepth =0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{.extent{.width = static_cast<uint32_t>(mppWindow.getWindowSize().x), .height = static_cast<uint32_t>(mppWindow.getWindowSize().y)}};
    vkCmdSetScissor(cb, 0, 1, &scissor);


    
}

void MppRenderer::endSwapChainRenderPass(VkCommandBuffer cb)
{
    

    //finish render pass
    vkCmdEndRendering(cb);

    
}

void MppRenderer::endFrame(VkCommandBuffer cb)
{
    //zmieniamy layout obrazu na taki wymagany do wyświetlania
    VkImageMemoryBarrier2 barrierPresent{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = mppSwapChain->getCurrentSwapchainImage(imageIndex),
        .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT , .levelCount = 1, .layerCount = 1 }
    };
    VkDependencyInfo barrierPresentDependencyInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrierPresent
    };
    vkCmdPipelineBarrier2(cb, &barrierPresentDependencyInfo);

    // We don't need a barrier for the depth attachment, as we don't use that outside of this render pass.

    //end recording :DDDDDDDDDDDDDDDDDDDDDDDDDDDDD
    chk(vkEndCommandBuffer(cb));
    // to zmienie state buffera na executable


    auto result = mppSwapChain->submitCommandBuffer(cb, imageIndex);

    if (result < VK_SUCCESS) {
        if (result == VK_ERROR_OUT_OF_DATE_KHR || mppWindow.wasWindowResized()) {
            mppWindow.resetResizedFlag();
            recreateSwapChain();
            return;
        }
        std::cerr << "Vulkan call returned an error (" << result << ")\n";
        exit(result);
    }
}


void MppRenderer::updateShaderData(ShaderData& shaderData)
{
    memcpy(mppSwapChain->getCurrentShaderDataBuffer().allocationInfo.pMappedData, &shaderData, sizeof(ShaderData));
}


void MppRenderer::recreateSwapChain()
{
    chk(vkDeviceWaitIdle(mppDevice.getLogicalDevice()));//czekanie aż gpu skończy działać
    std::shared_ptr<MppSwapChain> oldSwapChain = std::move(mppSwapChain);
    mppSwapChain = std::make_unique<MppSwapChain>(
        mppDevice, mppWindow, oldSwapChain
    );
    
    
}

    
} // namespace mpp
