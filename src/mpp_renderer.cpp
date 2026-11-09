#include "mpp_renderer.hpp"

namespace mpp
{


MppRenderer::MppRenderer(MppDevice& device, MppWindow& window): mppDevice{device}, mppWindow{window}
{
    recreateSwapChain();
    createCommandBuffers();
    createShaderDataBuffers();
    createSynchronizationObjects();
    //mppSwapChain = std::make_unique<MppSwapChain>(mppDevice, mppWindow);
}

MppRenderer::~MppRenderer()
{
    vkDeviceWaitIdle(mppDevice.getLogicalDevice());

    freeCommandBuffers();
    for (auto i = 0; i < MppSwapChain::MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vmaDestroyBuffer(mppDevice.getAllocator(), shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
        vkDestroyFence(mppDevice.getLogicalDevice(), fences[i], nullptr);
        vkDestroySemaphore(mppDevice.getLogicalDevice(), imageAcquiredSemaphores[i], nullptr);
    }
}

void MppRenderer::createCommandBuffers()
{
    // COMMAND BUFFERS ==============================================================



    /*
    It's not uncommon to have multiple command pools in a more complex application. 
    They're cheap to create and if you want to record command buffers from multiple 
    threads you require one such pool per thread.
    */

    VkCommandBufferAllocateInfo cbAllocCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mppDevice.getRenderingCommandPool(),
        .commandBufferCount = static_cast<uint32_t>(commandBuffers.size())
    };
    chk(vkAllocateCommandBuffers(mppDevice.getLogicalDevice(), &cbAllocCI, commandBuffers.data()), "failed to allocate command buffers!");
}

void MppRenderer::freeCommandBuffers()
{
    vkFreeCommandBuffers( mppDevice.getLogicalDevice(), mppDevice.getRenderingCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data() );
    //commandBuffers.clear();
}

void MppRenderer::recreateSwapChain()
{
    auto extent = mppWindow.getExtent();
    if (extent.width == 0 || extent.height == 0) {
        return;
    }
    

    vkDeviceWaitIdle(mppDevice.getLogicalDevice());

    if (mppSwapChain == nullptr) 
    {
        mppSwapChain = std::make_unique<MppSwapChain>(mppDevice, extent);
    } 
    else 
    {
        std::shared_ptr<MppSwapChain> oldSwapChain = std::move(mppSwapChain);
        mppSwapChain = std::make_unique<MppSwapChain>(mppDevice, extent, oldSwapChain);

        chk(oldSwapChain->compareSwapFormats(*mppSwapChain.get()), "Swap chain image(or depth) format has changed!");//TODO: Naprawić przedwczesne destroy old swapchain
    }
}

VkCommandBuffer MppRenderer::beginFrame()
{
    assert(!isFrameStarted && "Can't call beginFrame while already in progress");

    if (mppWindow.wasWindowResized()) {
        mppWindow.resetWindowResizedFlag();
        //std::cout << "[DEBUG] recreateSwapChain triggered by: beginFrame wasWindowResized\n";
        recreateSwapChain();
        return nullptr; 
    }

    VkExtent2D extent = mppWindow.getExtent();
    if (extent.width == 0 || extent.height == 0) {
        return nullptr; 
    }

    
    
    auto result = mppSwapChain->acquireNextImage(currentImageIndex, fences[frameIndex], imageAcquiredSemaphores[frameIndex]);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) 
    {
        //std::cout << "[DEBUG] recreateSwapChain triggered by: acquireNextImage VK_ERROR_OUT_OF_DATE_KHR\n";
        recreateSwapChain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    isFrameStarted=true;

    
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
    auto cb = getCurrentCommandBuffer();
    chk(vkResetCommandBuffer(cb,0), "failed to reset command buffer");

    //rozpoczęcie nagrywania
    VkCommandBufferBeginInfo cbBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    chk(vkBeginCommandBuffer(cb, &cbBI), "failed to begin recording command buffer!");

    return cb;
}

void MppRenderer::beginSwapChainRenderPass(VkCommandBuffer cb)
{
    assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
    assert(cb == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

    VkExtent2D swapChainExtent = mppSwapChain->getSwapChainExtent();

    //sprawdzanie czy jest stencil, jak nie to sie nie dodaje jego flagi
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (hasStencilComponent(mppSwapChain->getDepthFormat())) {
        aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

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
            .image = mppSwapChain->getCurrentSwapchainImage(currentImageIndex),
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
                .levelCount = 1, 
                .layerCount = 1 }
        },
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE, //czekaj an tego stage'a
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .image = mppSwapChain->getDepthImage(frameIndex),
            .subresourceRange{
                .aspectMask = aspectMask, 
                .levelCount = 1, 
                .layerCount = 1 }
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
        .imageView = mppSwapChain->getCurrentSwapchainImageView(currentImageIndex),
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue{.color{0.05f, 0.05f, 0.05f, 1.0f}}
    };
    VkRenderingAttachmentInfo depthAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = mppSwapChain->getDepthImageView(frameIndex),
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue{.depthStencil = {1.0f, 0}}
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea{.offset = {0,0}, .extent= swapChainExtent},
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
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth =0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cb, 0, 1, &vp);
    VkRect2D scissor{.offset = {0,0}, .extent =swapChainExtent};
    vkCmdSetScissor(cb, 0, 1, &scissor);
}

void MppRenderer::endSwapChainRenderPass(VkCommandBuffer cb)
{
    assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
    assert(cb == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");


    //finish render pass
    vkCmdEndRendering(cb);
}

void MppRenderer::endFrame(VkCommandBuffer cb)
{
    assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
    assert(cb == getCurrentCommandBuffer() && "Can't end frrame on command buffer from a different frame");


    //zmieniamy layout obrazu na taki wymagany do wyświetlania
    VkImageMemoryBarrier2 barrierPresent{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = mppSwapChain->getCurrentSwapchainImage(currentImageIndex),
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
    chk(vkEndCommandBuffer(cb), "failed to record command buffer!");
    // to zmienie state buffera na executable


    auto result = mppSwapChain->submitCommandBuffer(cb, currentImageIndex, fences[frameIndex] , imageAcquiredSemaphores[frameIndex]);

    // if (result == VK_SUBOPTIMAL_KHR) {
    //     std::cout << "[DEBUG] recreateSwapChain triggered by: VK_SUBOPTIMAL_KHR\n";
    // } else if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    //     std::cout << "[DEBUG] recreateSwapChain triggered by: VK_ERROR_OUT_OF_DATE_KHR\n";
    // } else if (mppWindow.wasWindowResized()) {
    //     std::cout << "[DEBUG] recreateSwapChain triggered by: window resize event\n";
    // }

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
    } 
    else if (result != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    isFrameStarted=false;
    frameIndex = (frameIndex +1) % MppSwapChain::MAX_FRAMES_IN_FLIGHT;
}


void MppRenderer::updateShaderData(ShaderData& shaderData)
{
    memcpy(getCurrentShaderDataBuffer().allocationInfo.pMappedData, &shaderData, sizeof(ShaderData));
}

bool MppRenderer::hasStencilComponent(VkFormat format) const
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || 
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}


void MppRenderer::createShaderDataBuffers()
{
    // CPU AND GPU PARALLELISM ========================================================

    /*
    The concept of frames in flight only applies to resource shared by CPU and GPU. 
    Resources that are only used by the GPU don't have to be multiplied. 
    This applies to e.g. images.
    */    

    // SHADER DATA BUFFERS ============================================================

    // wyżej struct na dane do shaderów
    // nie będziemy robić descriptorów itd bo jest jakiś inny feature co działa przez wskaźnik

    for (auto i = 0; i < MppSwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo uBufferCI{
            .sType =VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(ShaderData),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | 
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | //nie wiem czy ta flaga potrzebna
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT   //nie wiem czy ta flaga potrzebna
        };
        VmaAllocationCreateInfo uBufferAllocCI{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
            // VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | //zakomentowane bo to zezwolenie dla vma aby alokowało pamięć NIE host-visible(wtedy są potrzebne descriptory) TODO: obsłóżyć to jak sie chce wsteczną kompatybilność
            VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        chk(vmaCreateBuffer(mppDevice.getAllocator(), &uBufferCI, &uBufferAllocCI, &shaderDataBuffers[i].buffer, &shaderDataBuffers[i].allocation, &shaderDataBuffers[i].allocationInfo), 
            "Failed to allocate shader data buffer via VMA");
        
        VkBufferDeviceAddressInfo uBufferBdaInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = shaderDataBuffers[i].buffer
        };
        /*To be able to access the buffer in our shader, we then get its device address and store it for later access.*/
        shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(mppDevice.getLogicalDevice(), &uBufferBdaInfo);
        
        chk(shaderDataBuffers[i].deviceAddress != 0, "Failed to get Buffer Device Address! Ensure bufferDeviceAddress feature is enabled on VkDevice.");
    }
}

void MppRenderer::createSynchronizationObjects()
{
    // SYNCHRONIZATION OBJECTS =====================================================

    // fences, binary semaphores and pipeline barrirers
    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT // Zapobiega zablokowaniu przy pierwszej klatce
    };
    for ( auto i = 0; i < MppSwapChain::MAX_FRAMES_IN_FLIGHT; i++){
        chk(vkCreateFence(mppDevice.getLogicalDevice(), &fenceCI, nullptr, &fences[i]));
        chk(vkCreateSemaphore(mppDevice.getLogicalDevice(), &semaphoreCI, nullptr, &imageAcquiredSemaphores[i]));
    }
}
    
} // namespace mpp
