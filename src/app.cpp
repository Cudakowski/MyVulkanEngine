#include "app.hpp"



namespace mpp
{

void App::initialize()
{
    // Make sure asset folder is present from the current working directory
	if (!std::filesystem::is_directory("assets")) {
		std::cerr << "Could not locate assets folder from current working directory\n";
		exit(-1);
	}
	chk(SDL_Init(SDL_INIT_VIDEO));
	chk(SDL_Vulkan_LoadLibrary(NULL));
	volkInitialize();
}

App::App(/* args */)
{
    initialize();



    // SWAPCHAIN =======================================

    // pobieranie właściwości powieżchni
    VkSurfaceCapabilitiesKHR surfaceCaps{};// tu jest zapisywany wynik
    chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));
    
    // pobieramy extent
    VkExtent2D swapchainExtent{ surfaceCaps.currentExtent };
    if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
        swapchainExtent = { .width = static_cast<uint32_t>(windowSize.x), .height=static_cast<uint32_t>(windowSize.y)};
    }

    // info
    const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB };
    VkSwapchainCreateInfoKHR swapchainCI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surfaceCaps.minImageCount,
        .imageFormat = imageFormat,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,//format kolorów
        .imageExtent{.width = swapchainExtent.width, .height = swapchainExtent.height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR//v-sync
    };
    chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
    
    /*
    The swapchain setup shown here is a bare minimum. In a 
    real-world application this part can be quite complicated, 
    as you might have to adjust it based on user settings. One 
    example would be HDR capable devices, where you'd need to use 
    a different image format and color space.
    */

    // pobieranie liczny images i resizowanie wektora z images
    uint32_t imageCount{ 0 };
    chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
    swapchainImages.resize(imageCount);
    // teraz pobieranie images oraz zmienianie wielkości wektora na views
    chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
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
        chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
    }

    // DEPTH ATTACHMENT =========================================

    // pobieramy formaty i jak wyłapiemy format depth to go zapisujemy
    std::vector<VkFormat> depthFormatList{VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
    for ( VkFormat& format : depthFormatList ){
        VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
        vkGetPhysicalDeviceFormatProperties2(devices[deviceIndex], format, &formatProperties);
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
        .extent{.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y), .depth = 1},
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
    chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));

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
    chk(vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView));

    /*
    Images in Vulkan are not accessed directly, but rather through 
    views, a common concept in programming. This adds flexibility 
    and allows different access patterns for the same image.
    */

    // LOADING MESHES ===========================================

    //łądowanie modelu :DDDD
    // Mesh data
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/suzanne.obj"));

    // zmieniamy pobrane dane na taki format jaki chcemy żeby potem załadować to do gpu
    const VkDeviceSize indexCount{shapes[0].mesh.indices.size()};
    std::vector<Vertex> vertices{};
    std::vector<uint16_t> indices{};
    //load vertex and index data
    for (auto& index : shapes[0].mesh.indices) {
        Vertex v{
            .pos = {attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2]},
            .normal = {attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2]},
            .uv = {attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]}
        };
        vertices.push_back(v);
        indices.push_back(indices.size());
    }
    /*
    The value of the position's and normal's y-axis, and the texture coordinate's 
    v-axis are flipped. This is done to accommodate for Vulkan's coordinate system. 
    Otherwise the model and the texture image would appear upside down.
    */


    // tworzenie buffera na vertex i index data
    VkDeviceSize vBufSize{sizeof(Vertex) * vertices.size()};
    VkDeviceSize iBufSize{sizeof(uint16_t) * indices.size()};
    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vBufSize + iBufSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };

    // używamy vma do alokacji pamięci buffera na vertex and index data
    VmaAllocationCreateInfo vBufferAllocCI{
        // flagi do resizable BAR !! :D
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    }; 
    VmaAllocationInfo vBufferAllocInfo{};
    chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation, &vBufferAllocInfo));
    
    /* VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
    Without this we'd have to create a so-called "staging" buffer on the host, 
    copy data to that buffer and then submit a buffer copy from staging to the 
    GPU side buffer using a command buffer. That would require a lot more code.
    */

    //kopiowanie danych do vramu
    memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
    memcpy(((char*)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(), iBufSize);

    // CPU AND GPU PARALLELISM ========================================================

    /*
    The concept of frames in flight only applies to resource shared by CPU and GPU. 
    Resources that are only used by the GPU don't have to be multiplied. 
    This applies to e.g. images.
    */    

    // SHADER DATA BUFFERS ============================================================

    // wyżej struct na dane go shaderów
    // nie będziemy robić descriptorów itd bo jest jakiś inny feature co działa przez wskaźnik

    for (auto i = 0; i < maxFramesInFlight; i++) {
        VkBufferCreateInfo uBufferCI{
            .sType =VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(ShaderData),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        };
        VmaAllocationCreateInfo uBufferAllocCI{
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
        };
        chk(vmaCreateBuffer(allocator, &uBufferCI, &uBufferAllocCI, &shaderDataBuffers[i].buffer, &shaderDataBuffers[i].allocation, &shaderDataBuffers[i].allocationInfo));
        VkBufferDeviceAddressInfo uBufferBdaInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = shaderDataBuffers[i].buffer
        };
        /*To be able to access the buffer in our shader, we then get its device address and store it for later access.*/
        shaderDataBuffers[i].deviceAddress = vkGetBufferDeviceAddress(device, &uBufferBdaInfo);
    }

    // SYNCHRONIZATION OBJECTS =====================================================

    // fences, binary semaphores and pipeline barrirers
    VkSemaphoreCreateInfo semaphoreCI{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceCI{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for ( auto i = 0; i < maxFramesInFlight; i++){
        chk(vkCreateFence(device, &fenceCI, nullptr, &fences[i]));
        chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &imageAcquiredSemaphores[i]));
    }
    renderCompleteSemaphores.resize(swapchainImages.size());
    for (auto &semaphore : renderCompleteSemaphores)
    {
        chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
    }

    // COMMAND BUFFERS ==============================================================

    // buffery komend musza być zalokowane z command pool'a
    VkCommandPoolCreateInfo commandPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamily
    };
    chk(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));

    /*
    It's not uncommon to have multiple command pools in a more complex application. 
    They're cheap to create and if you want to record command buffers from multiple 
    threads you require one such pool per thread.
    */

    VkCommandBufferAllocateInfo cbAllocCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = maxFramesInFlight
    };
    chk(vkAllocateCommandBuffers(device, &cbAllocCI, commandBuffers.data()));

    // LOADING TEXTURES ============================================================

    //używany będzie ty format ktx, oficjalny od khronosa w formacie gpu'owym
    std::vector<VkDescriptorImageInfo> textureDescriptors{};
    for ( auto i = 0; i < textures.size(); i++) {
        ktxTexture* ktxTexture{ nullptr };
        std::string filename = "assets/suzanne" + std::to_string(i) + ".ktx";
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
        chk(vmaCreateImage(allocator, &texImgCI, &texImageAllocCI, &textures[i].image, &textures[i].allocation, nullptr));
        // tworzenie view. View image'a jest dla gpu do dostępu/spoglądania na zdjęcie
        VkImageViewCreateInfo texVewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = textures[i].image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = texImgCI.format,
            .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = ktxTexture->numLevels, .layerCount = 1}
        };
        chk(vkCreateImageView(device, &texVewCI, nullptr, &textures[i].view));
        
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
        chk(vmaCreateBuffer(allocator, &imgSrcBufferCI, &imgSrcAllocCI, &imgSrcBuffer, &imgSrcAllocation, &imgSrcAllocInfo));
        memcpy(imgSrcAllocInfo.pMappedData, ktxTexture->pData, ktxTexture->dataSize);

        //robienie commandBuffera do skopiowania zdjęcia z tego tymczasowego buffera do gpu
        VkFenceCreateInfo fenceOneTimeCI{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };
        VkFence fenceOneTime{};
        chk(vkCreateFence(device, &fenceOneTimeCI, nullptr, &fenceOneTime));
        VkCommandBuffer cbOneTime{};
        VkCommandBufferAllocateInfo cbOneTimeAI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .commandBufferCount = 1
        };
        chk(vkAllocateCommandBuffers(device, &cbOneTimeAI, &cbOneTime));

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
            .image = textures[i].image,
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
        vkCmdCopyBufferToImage(cbOneTime, imgSrcBuffer, textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()),copyRegions.data());


        VkImageMemoryBarrier2 barrierTexRead{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
            .image = textures[i].image,
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

        chk(vkQueueSubmit2(queue, 1, &oneTimeSI, fenceOneTime));
        chk(vkWaitForFences(device, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
    
    
        vkDestroyFence(device, fenceOneTime, nullptr);
        vmaDestroyBuffer(allocator, imgSrcBuffer, imgSrcAllocation);

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
        chk(vkCreateSampler(device, &samplerCI, nullptr, &textures[i].sampler));

        ktxTexture_Destroy(ktxTexture);
        textureDescriptors.push_back({
            .sampler = textures[i].sampler,
            .imageView = textures[i].view,
            .imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        });
    }

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
        .descriptorCount = static_cast<uint32_t>(textures.size()),
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    VkDescriptorSetLayoutCreateInfo descLayoutTexCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descBindingFlags,
        .bindingCount = 1,
        .pBindings = &descLayoutBindingTex
    };
    chk(vkCreateDescriptorSetLayout(device, &descLayoutTexCI, nullptr, &descriptorSetLayoutTex));

    // descriptor pool
    VkDescriptorPoolSize poolSize{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(textures.size())
    };
    VkDescriptorPoolCreateInfo descPoolCI{
        .sType =VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets =1,
        .poolSizeCount =1,
        .pPoolSizes = &poolSize
    };
    chk(vkCreateDescriptorPool(device, &descPoolCI, nullptr, &descriptorPool));

    // alokowania descriptor setu z tego pool'a
    uint32_t variableDescCount{static_cast<uint32_t>(textures.size())};
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
    chk(vkAllocateDescriptorSets(device, &texDescSetAlloc, &descriptorSetTex));

    // descriptor set ma niezainicjalizowane dane i trzeba je zainicjalizować
    VkWriteDescriptorSet writeDescSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSetTex,
        .dstBinding = 0,
        .descriptorCount = static_cast<uint32_t>(textures.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = textureDescriptors.data()
    };
    vkUpdateDescriptorSets(device, 1, &writeDescSet, 0, nullptr);

    // LOADING SHADERS =====================================================
    
    // kumpilowanie slanga do SPIR-V w runtimie :D
    // SPIR-V to taki pośredniczący język shaderów który vulkan umie czytać. 
    // GLSL też do niego najpierw sie kompiluje.

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
        slangSession->loadModuleFromSource("triangle", "assets/shader.slang", nullptr, nullptr)
    };
    Slang::ComPtr<ISlangBlob> spirv;
    slangModule->getTargetCode(0, spirv.writeRef());

    // tworzenie modułu shadera (kontener na skompilowany SPRI-V)
    VkShaderModuleCreateInfo shaderModuleCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spirv->getBufferSize(),
        .pCode = (uint32_t*)spirv->getBufferPointer()
    };
    VkShaderModule shaderModule{};
    chk(vkCreateShaderModule(device, &shaderModuleCI, nullptr, &shaderModule));

    /* To do nowszej wersji vulkana
    The VK_KHR_maintenance5 extension, which became core with 
    Vulkan 1.4, deprecated shader modules. It allows direct 
    passing of VkShaderModuleCreateInfo to the pipeline's 
    shader stage create info.
    */

    // THE SHADER =============================================

    // (pacz shader w assetach)

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
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(VkDeviceAddress)
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayoutTex,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    chk(vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &pipelineLayout));

    // layout of the vertex data
    VkVertexInputBindingDescription vertexBinding{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> vertexAttributes{
        {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT },
        {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset  = offsetof(Vertex, normal) },
        {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset  = offsetof(Vertex, uv) },
    };

    /*
    Another option for accessing vertices in the shader is buffer 
    device address. That way we would skip the traditional vertex 
    attributes and manually fetch them in the shader using pointers. That's called "vertex pulling". On some devices that can be slower though, so we stick with the traditional way.
    */

    VkPipelineVertexInputStateCreateInfo vertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBinding,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size()),
        .pVertexAttributeDescriptions = vertexAttributes.data()
    };

    //primitives
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // podłaćzenie shaderów do pipeline'a
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_VERTEX_BIT,
         .module = shaderModule, .pName = "main" },
        {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
         .module = shaderModule, .pName = "main" }
    };  

    /*
    If you'd wanted to use different shaders 
    (or shader combinations), you'd have to create multiple 
    pipelines. The VK_EXT_shader_objects makes those shader 
    stages into separate objects and adds more flexibility 
    to this part of the API.
    */

    //viewport (dynamic - nie trzeba recreate pipeline kiedy się zmienią)
    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount =1,
        .scissorCount =1
    };
    std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates.data()
    };

    //depth settings
    VkPipelineDepthStencilStateCreateInfo depthStencilState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };

    //dynamic rendering zamiast render pass objects :DDD
    VkPipelineRenderingCreateInfo renderingCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount =1,
        .pColorAttachmentFormats = &imageFormat,
        .depthAttachmentFormat =depthFormat
    };

    //nie będzie to używane ale trzeba to uspecyfikować jakimiś domyślnymi wartościami
    VkPipelineColorBlendAttachmentState blendAttachment{
        .colorWriteMask = 0xF
    };
    VkPipelineColorBlendStateCreateInfo colorBlendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendAttachment
    };
    VkPipelineRasterizationStateCreateInfo rasterizationState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0f
    };
    VkPipelineMultisampleStateCreateInfo multisampleState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    //creating pipeline :DDD
    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCI,
        .stageCount = 2,
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout
    };
    chk(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));


    // RENDER LOOP =============================================
    // :DD

    uint64_t lastTime{ SDL_GetTicks() };
    bool quit{ false };
    while (!quit) {
        // Wait on fence ********************************
        //czekanie na to aż gpu skończy wykonywać poprzedni commandbuffer
        chk(vkWaitForFences(device, 1, &fences[frameIndex], true, UINT64_MAX));
        chk(vkResetFences(device, 1, &fences[frameIndex]));

        // Acquire next image ********************************
        chkSwapchain(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex));

        // Update shader data ********************************
        shaderData.projection = glm::perspective(glm::radians(45.0f), (float)windowSize.x / (float)windowSize.y, 0.1f, 32.0f);
        shaderData.view = glm::translate(glm::mat4(1.0f), camPos);
        for (auto i = 0; i<3; i++) {
            auto instancePos = glm::vec3((float)(i-1)*3.0f, 0.0f, 0.0f);
            shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i]));
        }
        memcpy(shaderDataBuffers[frameIndex].allocationInfo.pMappedData, &shaderData, sizeof(ShaderData));
        
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
        auto cb = commandBuffers[frameIndex];
        chk(vkResetCommandBuffer(cb,0));

        //rozpoczęcie nagrywania
        VkCommandBufferBeginInfo cbBI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        chk(vkBeginCommandBuffer(cb, &cbBI));

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
                .image = swapchainImages[imageIndex],
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
                .image = depthImage,
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
            .imageView = swapchainImageViews[imageIndex],
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue{.color{0.0f, 0.0f, 0.2f, 1.0f}}
        };
        VkRenderingAttachmentInfo depthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = depthImageView,
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue{.depthStencil = {1.0f, 0}}
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{.extent{.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y)}},
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
            .width = static_cast<float>(windowSize.x),
            .height = static_cast<float>(windowSize.y),
            .minDepth =0.0f,
            .maxDepth = 1.0f
        };
        vkCmdSetViewport(cb, 0, 1, &vp);
        VkRect2D scissor{.extent{.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y)}};
        vkCmdSetScissor(cb, 0, 1, &scissor);

        //3D object data
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        VkDeviceSize vOffset{ 0 };
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSetTex, 0, nullptr);
        vkCmdBindVertexBuffers(cb, 0, 1, &vBuffer, &vOffset);
        vkCmdBindIndexBuffer(cb, vBuffer, vBufSize, VK_INDEX_TYPE_UINT16);

        //access to data in shader data buffer(to to takie co resizable bar że przez wskaźnik że nie trzeba descriptorów)
        vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkDeviceAddress), &shaderDataBuffers[frameIndex].deviceAddress);

        /*
        These vkCmd* calls (and many others) set the current 
        command buffer state. That means they persist across 
        multiple draw calls inside this command buffer. So if 
        you e.g. wanted to issue a second draw call with the 
        same pipeline but a different descriptor sets you'd 
        only have to call vkCmdBindDescriptorSets with another 
        set, while keeping the rest of the state.
        */

        //DRAW :DDDDD
        vkCmdDrawIndexed(cb, indexCount, 3, 0, 0, 0);

        //finish render pass
        vkCmdEndRendering(cb);

        //zmieniamy layout obrazu na taki wymagany do wyświetlania
        VkImageMemoryBarrier2 barrierPresent{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = swapchainImages[imageIndex],
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
        chk(vkQueueSubmit2(queue, 1, &submitInfo, fences[frameIndex]));
        
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
        frameIndex = (frameIndex + 1) % maxFramesInFlight;


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
        chkSwapchain(vkQueuePresentKHR(queue, &presentInfo));

        // Poll events ********************************

        float elapsedTime{ (SDL_GetTicks()- lastTime) / 1000.0f };
        lastTime = SDL_GetTicks();
        for (SDL_Event event; SDL_PollEvent(&event);)
        {
            //exit loop if ablication is about to close
            if(event.type == SDL_EVENT_QUIT)
            {
                quit = true;
                break;
            }

            //Rotate the selected object with mouse drag
            if (event.type == SDL_EVENT_MOUSE_MOTION) 
            {
                if(event.button.button == SDL_BUTTON_LEFT) 
                {
                    objectRotations[shaderData.selected].x -= (float)event.motion.yrel * elapsedTime;
                    objectRotations[shaderData.selected].y += (float)event.motion.xrel * elapsedTime;
                }
            }

            // Zooming with the mouse wheel
            if (event.type == SDL_EVENT_MOUSE_WHEEL)
            {
                camPos.z += (float)event.wheel.y * elapsedTime * 10.0f;
            }

            // Select active model instance
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
                {
                    shaderData.selected = (shaderData.selected < 2) ? shaderData.selected + 1 : 0;
                }
                if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
                {
                    shaderData.selected = (shaderData.selected > 0) ? shaderData.selected - 1 : 2;
                }
            }

            //window resize
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                updateSwapchain = true;
            }
        }

        // Recreate swapchain **********************************

        if (updateSwapchain)
        {
            chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));

            updateSwapchain = false;
            chk(vkDeviceWaitIdle(device));//czekanie aż gpu skończy działać
            
            chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));
            swapchainCI.oldSwapchain = swapchain;
            swapchainCI.imageExtent = {.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y)};
            chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));

            for(auto i = 0; i< imageCount; i++)
            {
                vkDestroyImageView(device, swapchainImageViews[i], nullptr);
            }

            chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
            swapchainImages.resize(imageCount);
            chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
            
            swapchainImageViews.resize(imageCount);

            for(auto i = 0; i< imageCount; i++)
            {
                VkImageViewCreateInfo viewCI{
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .image = swapchainImages[i],
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = imageFormat,
                    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
                };
                chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
            }

            for(auto& semaphore : renderCompleteSemaphores)
            {
                vkDestroySemaphore(device, semaphore, nullptr);
            }
            renderCompleteSemaphores.resize(imageCount);
            for(auto& semaphore : renderCompleteSemaphores)
            {
                chk(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
            }

            vkDestroySwapchainKHR(device, swapchainCI.oldSwapchain, nullptr);
            
            vmaDestroyImage(allocator, depthImage, depthImageAllocation);
            vkDestroyImageView(device, depthImageView, nullptr);

            depthImageCI.extent = {.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y), .depth = 1 };
            VmaAllocationCreateInfo allocCI{
                .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO
            };
            chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));
            
            VkImageViewCreateInfo viewCI{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = depthImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = depthFormat,
                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
            };
            chk(vkCreateImageView(device, &viewCI, nullptr, &depthImageView));
        }
    }

    // CLEANING UP ===========================================

    chk(vkDeviceWaitIdle(device));

	for (auto i = 0; i < maxFramesInFlight; i++) 
    {
		vkDestroyFence(device, fences[i], nullptr);
		vkDestroySemaphore(device, imageAcquiredSemaphores[i], nullptr);
		vmaDestroyBuffer(allocator, shaderDataBuffers[i].buffer, shaderDataBuffers[i].allocation);
	}

	for (auto i = 0; i < renderCompleteSemaphores.size(); i++) 
    {
		vkDestroySemaphore(device, renderCompleteSemaphores[i], nullptr);
	}

	vmaDestroyImage(allocator, depthImage, depthImageAllocation);
	vkDestroyImageView(device, depthImageView, nullptr);

	for (auto i = 0; i < swapchainImageViews.size(); i++) 
    {
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	}

	vmaDestroyBuffer(allocator, vBuffer, vBufferAllocation);

	for (auto i = 0; i < textures.size(); i++) 
    {
		vkDestroyImageView(device, textures[i].view, nullptr);
		vkDestroySampler(device, textures[i].sampler, nullptr);
		vmaDestroyImage(allocator, textures[i].image, textures[i].allocation);
	}

	vkDestroyDescriptorSetLayout(device, descriptorSetLayoutTex, nullptr);
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyShaderModule(device, shaderModule, nullptr);
	vmaDestroyAllocator(allocator);
	SDL_DestroyWindow(window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);



}

App::~App()
{

}

void App::run()
{

}




} // namespace mpp


