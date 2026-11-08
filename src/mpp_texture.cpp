#include "mpp_texture.hpp"


namespace mpp
{

MppTexture::MppTexture(MppDevice& device): mppDevice{device} 
{

}

MppTexture::~MppTexture()
{
    vkDestroyImageView(mppDevice.getLogicalDevice(), view, nullptr);
    vkDestroySampler(mppDevice.getLogicalDevice(), sampler, nullptr);
    vmaDestroyImage(mppDevice.getAllocator(), image, allocation);
}



std::unique_ptr<MppTexture> MppTexture::createTextureFromFile(MppDevice& mppDevice ,const std::string &filepath)
{

    ktxTexture* ktxTexture{ nullptr };
    auto ktxRes = ktxTexture_CreateFromNamedFile(filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
    if (ktxRes != KTX_SUCCESS || ktxTexture == nullptr) {
        throw std::runtime_error("Loading KTX texture failed: " + filepath);
    }

    auto mppTexture = std::make_unique<MppTexture>(mppDevice);

    VkImageCreateInfo texImgCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = ktxTexture_GetVkFormat(ktxTexture),
        .extent = {.width = ktxTexture->baseWidth, .height = ktxTexture->baseHeight, .depth = 1},
        .mipLevels = ktxTexture->numLevels,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    VmaAllocationCreateInfo texImageAllocCI{ .usage = VMA_MEMORY_USAGE_AUTO};
    chk(vmaCreateImage(mppDevice.getAllocator(), &texImgCI, &texImageAllocCI, &mppTexture->image, &mppTexture->allocation, nullptr));
    // tworzenie view. View image'a jest dla gpu do dostępu/spoglądania na zdjęcie
    VkImageViewCreateInfo texVewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = mppTexture->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = texImgCI.format,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1}
    };
    chk(vkCreateImageView(mppDevice.getLogicalDevice(), &texVewCI, nullptr, &mppTexture->view));
    
    // With the empty image created it's time to upload data
    // nie można skopiować danych tak jak wcześniej w bufferze z 
    // vertexami bo layout jest specyficzny na danym hardwearze 
    // dlatego najpierw się to kopiuje do tymczasowego buffera i 
    // tam komenda automatycznie robi konwersje tak jak chce.
    VkBuffer imgSrcBuffer{};
    VmaAllocation imgSrcAllocation{};
    VkBufferCreateInfo imgSrcBufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = static_cast<VkDeviceSize>(ktxTexture->dataSize),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    VmaAllocationCreateInfo imgSrcAllocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    VmaAllocationInfo imgSrcAllocInfo{};
    chk(vmaCreateBuffer(mppDevice.getAllocator(), &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo));
    memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData, ktxTexture->dataSize);

    //robienie commandBuffera do skopiowania zdjęcia z tego tymczasowego buffera do gpu
    VkFenceCreateInfo fenceOneTimeCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };
    VkFence fenceOneTime{};
    chk(vkCreateFence(mppDevice.getLogicalDevice(), &fenceOneTimeCI, nullptr, &fenceOneTime));
    VkCommandBuffer cbOneTime{};
    VkCommandBufferAllocateInfo cbOneTimeAI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mppDevice.getTextureLoadingCommandPool(),
        .commandBufferCount = 1
    };
    chk(vkAllocateCommandBuffers(mppDevice.getLogicalDevice(), &cbOneTimeAI, &cbOneTime));

    // nagrywanie komend
    VkCommandBufferBeginInfo cbOneTimeBI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    chk(vkBeginCommandBuffer(cbOneTime, &cbOneTimeBI));//begin


    VkImageMemoryBarrier2 barrierTexImage{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
        .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .image = mppTexture->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = ktxTexture->numLevels,
            .layerCount = 1
        }
    };
    VkDependencyInfo barrierTexInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrierTexImage
    };
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);//(transition mip levels) ustaw layout tak aby możno było do niego zapisywać


    std::vector<VkBufferImageCopy> copyRegions{};
    copyRegions.reserve(ktxTexture->numLevels);
    for (uint32_t j = 0; j < ktxTexture->numLevels; j++)
    {
        ktx_size_t mipOffset{0};
        ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);//TODO: obsługiwać błędy.
        copyRegions.push_back({
            .bufferOffset = mipOffset,
            .imageSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
                .mipLevel = j, 
                .layerCount = 1 
            },
            .imageExtent{
                .width = std::max(1u, ktxTexture->baseWidth >> j), //Dzielenie do zera: Jeśli szerokość lub wysokość spadnie poniżej 1 (np. przy teksturze niebędącej kwadratem, jak $512 \times 64$), 64 >> 7 da wynik 0. Vulkan zabrania width = 0 lub height = 0 w VkExtent3D. Należy zawsze wymusić co najmniej 1:
                .height = std::max(1u, ktxTexture->baseHeight >> j), 
                .depth = 1 
            } //TODO: Formaty kompresowane blokowo (BC1–BC7 / ASTC): Formaty KTX często używają kompresji blokowej (bloki $4 \times 4$ piksele). Wymiar mipmapy na poziomie Vulkan API nie może być mniejszy niż rozmiar bloku kompresji, chyba że tekstura jest odpowiednio dopasowana. Funkcja KTX posiada gotowe helpery, ale std::max(1u, ...) zabezpiecza przynajmniej bazowy przypadek.
        });
    }
    // zapisywanie z tymczasowego buffera
    vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, mppTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()),copyRegions.data());


    VkImageMemoryBarrier2 barrierTexRead{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        .image = mppTexture->image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, 
            .levelCount = ktxTexture->numLevels, 
            .layerCount = 1 
        }
    };
    barrierTexInfo.pImageMemoryBarriers = &barrierTexRead;
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);//(transition mip levels) ustaw layout tak aby możno było go odczytać w shaderze
    
    /*
    Extensions that would make this easier are 
    VK_EXT_host_image_copy, allowing for copying image data 
    directly from the CPU without having to use a command 
    buffer and VK_KHR_unified_image_layouts, simplifying image 
    layouts. These aren't widely supported yet, but future 
    candidates for making Vulkan easier to use.
    */
    
    chk(vkEndCommandBuffer(cbOneTime)); // end

    VkCommandBufferSubmitInfo cbOneTimeSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cbOneTime
    };
    VkSubmitInfo2 oneTimeSI{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cbOneTimeSubmitInfo
    };

    chk(vkQueueSubmit2(mppDevice.getGraphicsQueue(), 1, &oneTimeSI, fenceOneTime));
    chk(vkWaitForFences(mppDevice.getLogicalDevice(), 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
    
    vkDestroyFence(mppDevice.getLogicalDevice(), fenceOneTime, nullptr);
    vmaDestroyBuffer(mppDevice.getAllocator(), imgSrcBuffer, imgSrcAllocation);
    vkFreeCommandBuffers(mppDevice.getLogicalDevice(), mppDevice.getTextureLoadingCommandPool(), 1, &cbOneTime);//TODO: zrobić żeby tworzone były te obiekty dla wszystkich tekstur raz i niszczone raz (a przynajmniej mniej razy)

    //sampler
    VkSamplerCreateInfo samplerCI{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 8.0f, // 8 is a widely supported value for max anisotropy
        .maxLod = (float)ktxTexture->numLevels
    };
    chk(vkCreateSampler(mppDevice.getLogicalDevice(), &samplerCI, nullptr, &mppTexture->sampler));

    ktxTexture_Destroy(ktxTexture);
    mppTexture->textureDescriptor ={
        .sampler = mppTexture->sampler,
        .imageView = mppTexture->view,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
    };


    return std::move(mppTexture);
}

} // namespace mpp
