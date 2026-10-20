#include "mpp_swap_chain.hpp"

#include "mpp_checks.hpp"

namespace mpp
{

MppSwapChain::MppSwapChain(MppDevice &device, MppWindow &window, std::shared_ptr<MppSwapChain> previous): mppDevice{device}, mppWindow{window}, oldSwapChain{previous}
{
    createSwapChain();

    for(auto i = 0; i< imageCount; i++)
    {
        vkDestroyImageView(mppDevice.getLogicalDevice(), swapchainImageViews[i], nullptr);
    }

    createImageViews();
    createShaderDataBuffers();
    
    for(auto& semaphore : renderCompleteSemaphores)
    {
        vkDestroySemaphore(mppDevice.getLogicalDevice(), semaphore, nullptr);
    }
    createSynchronizationObjects();
    
    vkDestroySwapchainKHR(mppDevice.getLogicalDevice(), oldSwapChain->swapchain, nullptr);
    vmaDestroyImage(mppDevice.getAllocator(), depthImage, depthImageAllocation);
    vkDestroyImageView(mppDevice.getLogicalDevice(), depthImageView, nullptr);
    
    createDepthAttachment();
    createCommandBuffers();
}

MppSwapChain::MppSwapChain(MppDevice &device, MppWindow &window): mppDevice{device}, mppWindow{window}
{
    createSwapChain();
    createImageViews();
    createDepthAttachment();
    createShaderDataBuffers();
    createSynchronizationObjects();
    createCommandBuffers();
}

VkResult MppSwapChain::acquireNextImage(uint32_t& imageIndex)
{
    // Wait on fence ********************************
    //czekanie na to aż gpu skończy wykonywać poprzedni commandbuffer
    chk(vkWaitForFences(mppDevice.getLogicalDevice(), 1, &fences[frameIndex], true, UINT64_MAX));
    chk(vkResetFences(mppDevice.getLogicalDevice(), 1, &fences[frameIndex]));

    // Acquire next image ********************************
    return vkAcquireNextImageKHR(mppDevice.getLogicalDevice(), swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
}

VkResult MppSwapChain::submitCommandBuffer(VkCommandBuffer cb,uint32_t& imageIndex)
{
    // Submit command buffer ********************************

    //submitowanie command buffera do queue
    VkSemaphoreSubmitInfo waitSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = imageAcquiredSemaphores[frameIndex],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkCommandBufferSubmitInfo commandBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cb
    };
    VkSemaphoreSubmitInfo signalSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = renderCompleteSemaphores[imageIndex],
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkSubmitInfo2 submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitSemaphoreInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalSemaphoreInfo
    };
    chk(vkQueueSubmit2(mppDevice.getQueue(), 1, &submitInfo, fences[frameIndex]));
    
    /*
    The wait semaphore in pWaitSemaphoreInfos ensures 
    that execution of the command buffer does not begin 
    until the swapchain image we want to render to has 
    been acquired. This means presentation has finished 
    and the image has been released by the presentation 
    engine. This is required because swapchain images 
    are owned by the presentation engine rather than by 
    our application. The stage specified in stageMask 
    makes that wait happen at the color attachment output 
    stage, so in theory the GPU may already begin work 
    on earlier pipeline stages, such as vertex fetching. 
    The signal semaphore in pSignalSemaphoreInfos, on 
    the other hand, is signaled by the GPU once command 
    buffer execution has completed and ensures that 
    presentation does not begin until the command buffer 
    has finished execution. Together, these guarantees 
    prevent read/write hazards that could cause the GPU 
    to read from or write to resources that are 
    still in use.
    */

    //calculate index for the next loop iterationnnnnnnnnnn
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;


    // Present image ********************************

    //wstawianie na ekran :DDDD
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderCompleteSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };
    return vkQueuePresentKHR(mppDevice.getQueue(), &presentInfo);
}

MppSwapChain::~MppSwapChain()
{
    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
    {
        vkDestroyFence(mppDevice.getLogicalDevice(), fences[i], nullptr);
        vkDestroySemaphore(mppDevice.getLogicalDevice(), imageAcquiredSemaphores[i], nullptr);
        vmaDestroyBuffer(mppDevice.getAllocator(), shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
    }

    for (auto i = 0; i < renderCompleteSemaphores.size(); i++) 
    {
        vkDestroySemaphore(mppDevice.getLogicalDevice(), renderCompleteSemaphores[i], nullptr);
    }

    vmaDestroyImage(mppDevice.getAllocator(), depthImage, depthImageAllocation);
    vkDestroyImageView(mppDevice.getLogicalDevice(), depthImageView, nullptr);

    for (auto i = 0; i < swapchainImageViews.size(); i++) 
    {
        vkDestroyImageView(mppDevice.getLogicalDevice(), swapchainImageViews[i], nullptr);
    }

	vkDestroySwapchainKHR(mppDevice.getLogicalDevice(), swapchain, nullptr);
    
}

void MppSwapChain::createCommandBuffers()
{
    // COMMAND BUFFERS ==============================================================



    /*
    It's not uncommon to have multiple command pools in a more complex application. 
    They're cheap to create and if you want to record command buffers from multiple 
    threads you require one such pool per thread.
    */

    VkCommandBufferAllocateInfo cbAllocCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mppDevice.getCommandPool(),
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    chk(vkAllocateCommandBuffers(mppDevice.getLogicalDevice(), &cbAllocCI, commandBuffers.data()));
}

void MppSwapChain::createSynchronizationObjects()
{
    // SYNCHRONIZATION OBJECTS =====================================================

    // fences, binary semaphores and pipeline barrirers
    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for ( auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        chk(vkCreateFence(mppDevice.getLogicalDevice(), &fenceCI, nullptr, &fences[i]));
        chk(vkCreateSemaphore(mppDevice.getLogicalDevice(), &semaphoreCI, nullptr, &imageAcquiredSemaphores[i]));
    }
    renderCompleteSemaphores.resize(swapchainImages.size());
    for (auto &semaphore : renderCompleteSemaphores)
    {
        chk(vkCreateSemaphore(mppDevice.getLogicalDevice(), &semaphoreCI, nullptr, &semaphore));
    }
}


void MppSwapChain::createShaderDataBuffers()
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

    for (auto i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo uBufferCI{
            .sType =VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(ShaderData),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
        VmaAllocationCreateInfo uBufferAllocCI{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        chk(vmaCreateBuffer(mppDevice.getAllocator(), &uBufferCI, &uBufferAllocCI, &shaderDataBuffers[i].buffer, &shaderDataBuffers[i].allocation, &shaderDataBuffers[i].allocationInfo));
        VkBufferDeviceAddressInfo uBufferBdaInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = shaderDataBuffers[i].buffer
        };
        /*To be able to access the buffer in our shader, we then get its device address and store it for later access.*/
        shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(mppDevice.getLogicalDevice(), &uBufferBdaInfo);
    }
}

void MppSwapChain::createDepthAttachment()
{

    // pobieramy formaty i jak wyłapiemy format depth to go zapisujemy
    std::vector<VkFormat> depthFormatList{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    for ( VkFormat& format : depthFormatList ){
        VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
        vkGetPhysicalDeviceFormatProperties2(mppDevice.getPhysicalDevice(), format, &formatProperties);
        if(formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT){
            depthFormat = format;
            break;
        }
    }

    // info depth image
    VkImageCreateInfo depthImageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent{.width = static_cast<uint32_t>(mppWindow.getWindowSize().x), .height = static_cast<uint32_t>(mppWindow.getWindowSize().y), .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    // info alokacji
    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO//automatycznie wykrywa użycie :DD
    };
    // tworzenie image(alokacja tym alokatorem vma)
    chk(vmaCreateImage(mppDevice.getAllocator(), &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));

    /*
    We only need a single image, even if we do double buffering in 
    other places. That's because the image is only ever accessed by 
    the GPU and the GPU can only ever write to a single depth image 
    at a time. This differs from resources shared by the CPU and the 
    GPU, but more on that later.
    */

    // info view tego image'a
    VkImageViewCreateInfo depthViewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = depthImage,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = depthFormat,
        .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
    };
    // tworzenie image view, przyjmuje rzeczy jak zwykle
    chk(vkCreateImageView(mppDevice.getLogicalDevice(), &depthViewCI, nullptr, &depthImageView));

    /*
    Images in Vulkan are not accessed directly, but rather through 
    views, a common concept in programming. This adds flexibility 
    and allows different access patterns for the same image.
    */
}

void MppSwapChain::createSwapChain()
{
    swapchainExtent = mppWindow.getExtent();

    // info
    
    VkSwapchainCreateInfoKHR swapchainCI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = mppWindow.getSurface(),
        .minImageCount = mppWindow.getSurfaceCaps().minImageCount,
        .imageFormat = imageFormat,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,//format kolorów
        .imageExtent{.width = swapchainExtent.width, .height = swapchainExtent.height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR//v-sync
    };

    swapchainCI.oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapchain;

    chk(vkCreateSwapchainKHR(mppDevice.getLogicalDevice(), &swapchainCI, nullptr, &swapchain));
    
    /*
    The swapchain setup shown here is a bare minimum. In a 
    real-world application this part can be quite complicated, 
    as you might have to adjust it based on user settings. One 
    example would be HDR capable devices, where you'd need to use 
    a different image format and color space.
    */
}

void MppSwapChain::createImageViews()
{
    // pobieranie liczby images i resizowanie wektora z images
    chk(vkGetSwapchainImagesKHR(mppDevice.getLogicalDevice(), swapchain, &imageCount, nullptr));
    swapchainImages.resize(imageCount);
    // teraz pobieranie images oraz zmienianie wielkości wektora na views
    chk(vkGetSwapchainImagesKHR(mppDevice.getLogicalDevice(), swapchain, &imageCount, swapchainImages.data()));
    swapchainImageViews.resize(imageCount);
    for (auto i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageFormat,
            .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
        };  
        chk(vkCreateImageView(mppDevice.getLogicalDevice(), &viewCI, nullptr, &swapchainImageViews[i]));
    }
}


} // namespace mpp
