#include "mpp_swap_chain.hpp"


namespace mpp
{


MppSwapChain::MppSwapChain(MppDevice &device, VkExtent2D winExtent): mppDevice{device}, windowExtent{winExtent}
{
    init();
}

MppSwapChain::MppSwapChain(MppDevice &device, VkExtent2D winExtent,std::shared_ptr<MppSwapChain> previous): mppDevice{device}, windowExtent{winExtent}, oldSwapChain{previous}
{
    init();
    oldSwapChain = nullptr;// TODO: standard wymaga żeby przy rekreowaniu swap chaina i przy podaniu starego żeby ten stary istniał do końca prezentowania swoich images i tu jest zbyt wcześnie niszczony
}

void MppSwapChain::init()
{
    createSwapChain();
    createImageViews();
    createDepthAttachment();
    createSynchronizationObjects();
}

MppSwapChain::~MppSwapChain()
{
    

    for (size_t i = 0; i < renderCompleteSemaphores.size(); i++) 
    {
        vkDestroySemaphore(mppDevice.getLogicalDevice(), renderCompleteSemaphores[i], nullptr);
    }

    for (int i= 0 ; i< MAX_FRAMES_IN_FLIGHT ; i++)
    {
        vkDestroyImageView(mppDevice.getLogicalDevice(), depthImageViews[i], nullptr);
        vmaDestroyImage(mppDevice.getAllocator(), depthImages[i], depthImageAllocations[i]);
    }    


    for (size_t i = 0; i < swapchainImageViews.size(); i++) 
    {
        vkDestroyImageView(mppDevice.getLogicalDevice(), swapchainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(mppDevice.getLogicalDevice(), swapchain, nullptr);
    
}


VkResult MppSwapChain::acquireNextImage(uint32_t& imageIndex, VkFence fence, VkSemaphore imageAcquiredSemaphore)const
{
    // Wait on fence ********************************
    //czekanie na to aż gpu skończy wykonywać poprzedni commandbuffer
    chk(vkWaitForFences(mppDevice.getLogicalDevice(), 1, &fence, true, UINT64_MAX));

    // Acquire next image ********************************
    auto result = vkAcquireNextImageKHR(mppDevice.getLogicalDevice(), swapchain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        chk(vkResetFences(mppDevice.getLogicalDevice(), 1, &fence));
    }

    return result;
}

VkResult MppSwapChain::submitCommandBuffer(VkCommandBuffer cb,uint32_t& imageIndex, VkFence fence, VkSemaphore imageAcquiredSemaphore)const
{
    // Submit command buffer ********************************

    //submitowanie command buffera do queue
    VkSemaphoreSubmitInfo waitSemaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = imageAcquiredSemaphore,
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
    chk(vkQueueSubmit2(mppDevice.getGraphicsQueue(), 1, &submitInfo, fence));
    
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
    //frameIndexInternal = (frameIndexInternal + 1) % MAX_FRAMES_IN_FLIGHT;


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
    return vkQueuePresentKHR(mppDevice.getPresentQueue(), &presentInfo);
}





void MppSwapChain::createSynchronizationObjects()
{
    // SYNCHRONIZATION OBJECTS =====================================================

    // fences, binary semaphores and pipeline barrirers
    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    renderCompleteSemaphores.resize(swapchainImages.size());
    for (auto &semaphore : renderCompleteSemaphores)
    {
        chk(vkCreateSemaphore(mppDevice.getLogicalDevice(), &semaphoreCI, nullptr, &semaphore));
    }
}




void MppSwapChain::createDepthAttachment()
{
    depthFormat = findDepthFormat(); // jest tylko jeden, a powinno być MAX_RAMES_IN_FLIGHT

    // pobieramy formaty i jak wyłapiemy format depth to go zapisujemy
    // std::vector<VkFormat> depthFormatList{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    // for ( VkFormat& format : depthFormatList ){
    //     VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
    //     vkGetPhysicalDeviceFormatProperties2(mppDevice.getPhysicalDevice(), format, &formatProperties);
    //     if(formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT){
    //         depthFormat = format;
    //         break;
    //     }
    // }

    // info depth image
    VkImageCreateInfo depthImageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depthFormat,
        .extent{
            .width = swapChainExtent.width, 
            .height = swapChainExtent.height, 
            .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    // info alokacji
    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO//automatycznie wykrywa użycie :DD
    };
    // tworzenie image(alokacja tym alokatorem vma)
    // if(vmaCreateImage(mppDevice.getAllocator(), &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("Failed to allocate and create Depth Image via VMA!");
    // }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        chk(vmaCreateImage(mppDevice.getAllocator(), &depthImageCI, &allocCI, &depthImages[i], &depthImageAllocations[i], nullptr), "Failed to allocate and create Depth Image via VMA!");

        // info view tego image'a
        VkImageViewCreateInfo depthViewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = depthImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = depthFormat,
            .subresourceRange{ 
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, //jeżeli używa się tego tylko jako depth bez sencila to jest git. Jeżweli chce się używać stencila to trzeba to zmienić
                .baseMipLevel = 0,
                .levelCount = 1, 
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        chk(vkCreateImageView(mppDevice.getLogicalDevice(), &depthViewCI, nullptr, &depthImageViews[i]), "Failed to create Depth Image View");
    }

    /*
    "We only need a single image, even if we do double buffering in 
    other places. That's because the image is only ever accessed by 
    the GPU and the GPU can only ever write to a single depth image 
    at a time. This differs from resources shared by the CPU and the 
    GPU, but more on that later."

    Jeśli obie klatki korzystają z tego samego depthImage, GPU 
    mogłoby zacząć pisać do bufora głębokości nowej klatki, zanim 
    poprzednia skończyła go używać.

    Aby jeden depthImage działał w 100% poprawnie przy wielu 
    klatkach w locie, musisz mieć pewność, że:

    Albo synchronizacja semaforami/fence'ami klatek gwarantuje 
    brak nakładania się etapów EARLY/LATE_FRAGMENT_TESTS między 
    klatkami.

    Albo – co jest najczęstszą i najbezpieczniejszą praktyką – 
    tworzy się bufor głębokości nie na każdy obraz Swapchaina 
    (jak w starym kodzie), ale na każdą ramkę w locie 
    (MAX_FRAMES_IN_FLIGHT, zazwyczaj 2).
    */

    // // info view tego image'a
    // VkImageViewCreateInfo depthViewCI{
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    //     .image = depthImage,
    //     .viewType = VK_IMAGE_VIEW_TYPE_2D,
    //     .format = depthFormat,
    //     .subresourceRange{ 
    //         .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, //jeżeli używa się tego tylko jako depth bez sencila to jest git. Jeżweli chce się używać stencila to trzeba to zmienić
    //         .baseMipLevel = 0,
    //         .levelCount = 1, 
    //         .baseArrayLayer = 0,
    //         .layerCount = 1,
    //     }
    // };
    // tworzenie image view, przyjmuje rzeczy jak zwykle
    // if(vkCreateImageView(mppDevice.getLogicalDevice(), &depthViewCI, nullptr, &depthImageView) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("Failed to create Depth Image View");
    // }

    /*
    Images in Vulkan are not accessed directly, but rather through 
    views, a common concept in programming. This adds flexibility 
    and allows different access patterns for the same image.
    */
}

void MppSwapChain::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = mppDevice.getSwapChainSupport();
    
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);
    imageFormat = surfaceFormat.format;

    imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    QueueFamilyIndices indices = mppDevice.findPhysicalQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
    bool differentQueues = (indices.graphicsFamily != indices.presentFamily);

    // info
    VkSwapchainCreateInfoKHR swapchainCI = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = mppDevice.getSurface(),
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        //.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,//format kolorów
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = differentQueues ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = differentQueues ? 2u : 0u,
        .pQueueFamilyIndices   = differentQueues ? queueFamilyIndices : nullptr,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapchain
    };

    chk(vkCreateSwapchainKHR(mppDevice.getLogicalDevice(), &swapchainCI, nullptr, &swapchain), "failed to create swap chain");
    
    /*
    The swapchain setup shown here is a bare minimum. In a 
    real-world application this part can be quite complicated, 
    as you might have to adjust it based on user settings. One 
    example would be HDR capable devices, where you'd need to use 
    a different image format and color space.
    */

    // pobieranie liczby images i resizowanie wektora z images
    chk(vkGetSwapchainImagesKHR(mppDevice.getLogicalDevice(), swapchain, &imageCount, nullptr));
    swapchainImages.resize(imageCount);
    // teraz pobieranie images oraz zmienianie wielkości wektora na views
    chk(vkGetSwapchainImagesKHR(mppDevice.getLogicalDevice(), swapchain, &imageCount, swapchainImages.data()));
    
}

VkFormat MppSwapChain::findDepthFormat()
{
    return mppDevice.findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}


void MppSwapChain::createImageViews()
{
    
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo viewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
                .baseMipLevel = 0,
                .levelCount = 1, 
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };  
        chk(vkCreateImageView(mppDevice.getLogicalDevice(), &viewCI, nullptr, &swapchainImageViews[i]), "failed to create texture image view!");
    }
}

VkSurfaceFormatKHR MppSwapChain::chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats)const
{
    for (const auto &availableFormat : availableFormats) 
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR MppSwapChain::chooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes)const
{
    // for (const auto &availablePresentMode : availablePresentModes) {
    //   if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
    //     std::cout << "Present mode: Mailbox" << std::endl;
    //     return availablePresentMode;
    //   }
    // }

    // for (const auto &availablePresentMode : availablePresentModes) {
    //   if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
    //     std::cout << "Present mode: Immediate" << std::endl;
    //     return availablePresentMode;
    //   }
    // }

    std::cout << "Present mode: V-Sync" << std::endl;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D MppSwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = windowExtent;
        actualExtent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}


} // namespace mpp
