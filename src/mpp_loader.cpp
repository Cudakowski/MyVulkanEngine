#include "mpp_loader.hpp"

#include <filesystem>
#include <fstream>
#include <cstring>
#include <iostream>

namespace mpp
{

MppLoader::MppLoader(MppDevice& device): mppDevice{device}
{

}

MppLoader::~MppLoader()
{
	vkDestroyDescriptorSetLayout(mppDevice.getLogicalDevice(), descriptorSetLayoutTex, nullptr);
	vkDestroyDescriptorPool(mppDevice.getLogicalDevice(), descriptorPool, nullptr);
	vkDestroyShaderModule(mppDevice.getLogicalDevice(), shaderModule, nullptr);
}

void MppLoader::loadShaders()
{
    // LOADING SHADERS =====================================================
    
    // kumpilowanie slanga do SPIR-V w runtimie :D (TODO:: tylko na czas developement'u i do edytora(ryyko bezpieczeństwa))
    // SPIR-V to taki pośredniczący język shaderów który vulkan umie czytać. 
    // GLSL też do niego najpierw sie kompiluje.

    const std::filesystem::path sourcePath = "assets/shader.slang";
    const std::filesystem::path cachePath  = "assets/shader.spv";

    std::vector<uint32_t> spirvData;
    bool needsCompilation = true;


    //pobieranie już skompilowanego pliku jeżeli istnieje i nie jest stary
    if (std::filesystem::exists(cachePath) && std::filesystem::exists(sourcePath)) {
        auto sourceTime = std::filesystem::last_write_time(sourcePath);
        auto cacheTime  = std::filesystem::last_write_time(cachePath);

        if (cacheTime >= sourceTime) {
            std::ifstream file(cachePath, std::ios::binary | std::ios::ate);
            size_t size = file.tellg();
            file.seekg(0);
            spirvData.resize(size / sizeof(uint32_t));
            file.read(reinterpret_cast<char*>(spirvData.data()), size);
            needsCompilation = false;
            std::cout<<"Loaded shader\n";
        }
    }
    if(needsCompilation)
    {

        // tworzenie globalnej sesji
        slang::createGlobalSession(slangGlobalSession.writeRef());
        
        // tworzenie takiej o sesji xd
        auto slangTargets{ std::to_array<slang::TargetDesc>({ {
            .format{SLANG_SPIRV},
            .profile{slangGlobalSession->findProfile("spirv_1_4")}
        } })};
        auto slangOptions{std::to_array<slang::CompilerOptionEntry>({ {
            slang::CompilerOptionName::EmitSpirvDirectly,
            {slang::CompilerOptionValueKind::Int, 1}
        } })};
        slang::SessionDesc slangSessionDesc{
            .targets{slangTargets.data()},
            .targetCount{SlangInt(slangTargets.size())},
            .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
            .compilerOptionEntries{slangOptions.data()},
            .compilerOptionEntryCount{uint32_t(slangOptions.size())}
        };
        Slang::ComPtr<slang::ISession> slangSession;
        slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
    
        // loading texture shader
        Slang::ComPtr<slang::IModule> slangModule{
            slangSession->loadModuleFromSource("triangle", sourcePath.string().c_str(), nullptr, nullptr)
        };
        Slang::ComPtr<ISlangBlob> spirv;
        slangModule->getTargetCode(0, spirv.writeRef());
    
        //zapisywanie
        size_t byteSize = spirv->getBufferSize();
        spirvData.resize(byteSize / sizeof(uint32_t));
        std::memcpy(spirvData.data(), spirv->getBufferPointer(), byteSize);

        std::ofstream outFile(cachePath, std::ios::binary);
        outFile.write(reinterpret_cast<const char*>(spirv->getBufferPointer()), byteSize);
        std::cout<<"Compiled shader\n";
    }

    // tworzenie modułu shadera (kontener na skompilowany SPRI-V)
    VkShaderModuleCreateInfo shaderModuleCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirvData.size() * sizeof(uint32_t),
        .pCode = spirvData.data()
    };
    chk(vkCreateShaderModule(mppDevice.getLogicalDevice(), &shaderModuleCI, nullptr, &shaderModule));

    /* To do nowszej wersji vulkana
    The VK_KHR_maintenance5 extension, which became core with 
    Vulkan 1.4, deprecated shader modules. It allows direct 
    passing of VkShaderModuleCreateInfo to the pipeline's 
    shader stage create info.
    */

    // THE SHADER =============================================

    // (pacz shader w assetach)
}



void MppLoader::initializeTextures()
{
    // LOADING TEXTURES ============================================================

    //używany będzie tu format ktx, oficjalny od khronosa w formacie gpu'owym


    //Descriptorrrrr (z indexingiem)

    // to na razie sam layout
    VkDescriptorBindingFlags descVariableFlag{ VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT };
    VkDescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &descVariableFlag
    };
    VkDescriptorSetLayoutBinding descLayoutBindingTex{
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(textureCount),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descBindingFlags,
        .bindingCount = 1,
        .pBindings = &descLayoutBindingTex
    };
    chk(vkCreateDescriptorSetLayout(mppDevice.getLogicalDevice(), &descLayoutTexCI, nullptr, &descriptorSetLayoutTex));

    // descriptor pool
    VkDescriptorPoolSize poolSize{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(textureCount)
    };
    VkDescriptorPoolCreateInfo descPoolCI{
        .sType =VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets =1,
        .poolSizeCount =1,
        .pPoolSizes = &poolSize
    };
    chk(vkCreateDescriptorPool(mppDevice.getLogicalDevice(), &descPoolCI, nullptr, &descriptorPool));

    // alokowania descriptor setu z tego pool'a
    uint32_t variableDescCount{static_cast<uint32_t>(textureCount)};
    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountAI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .descriptorSetCount =1,
        .pDescriptorCounts = &variableDescCount
    };
    VkDescriptorSetAllocateInfo texDescSetAlloc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableDescCountAI,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptorSetLayoutTex
    };
    chk(vkAllocateDescriptorSets(mppDevice.getLogicalDevice(), &texDescSetAlloc, &descriptorSetTex));

    // descriptor set ma niezainicjalizowane dane i trzeba je zainicjalizować
    VkWriteDescriptorSet writeDescSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSetTex,
        .dstBinding = 0,
        .descriptorCount = static_cast<uint32_t>(textureCount),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = textureDescriptors.data()
    };
    vkUpdateDescriptorSets(mppDevice.getLogicalDevice(), 1, &writeDescSet, 0, nullptr);
}


std::unique_ptr<MppTexture> MppLoader::createTextureFromFile(const std::string &filename)
{

    ktxTexture* ktxTexture{ nullptr };
    auto mppTexture = std::make_unique<MppTexture>(mppDevice);

    ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
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
        .size = (uint32_t)ktxTexture->dataSize,
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
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
    };
    VkDependencyInfo barrierTexInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrierTexImage
    };
    vkCmdPipelineBarrier2(cbOneTime, &barrierTexInfo);//(transition mip levels) ustaw layout tak aby możno było do niego zapisywać


    std::vector<VkBufferImageCopy> copyRegions{};
    for (auto j = 0; j < ktxTexture->numLevels; j++)
    {
        ktx_size_t mipOffset{0};
        KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, j, 0, 0, &mipOffset);
        copyRegions.push_back({
            .bufferOffset = mipOffset,
            .imageSubresource{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = (uint32_t)j, .layerCount = 1 },
            .imageExtent{.width = ktxTexture->baseWidth >> j, .height = ktxTexture->baseHeight >> j, .depth = 1 }
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
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1 }
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
    textureDescriptors.push_back({
        .sampler = mppTexture->sampler,
        .imageView = mppTexture->view,
        .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
    });

    textureCount++;

    return std::move(mppTexture);
    // TODO: Zwolnić pamięc po buferze komend
}


    
} // namespace mpp
