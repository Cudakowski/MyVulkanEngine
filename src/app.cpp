#include "app.hpp"

#include "basic_render_system.hpp"

namespace mpp
{

void App::loadGameObjects()
{
    std::shared_ptr<MppModel> mppModel = mppLoader.createModelFromFile("assets/suzanne.obj");
    std::shared_ptr<MppTexture> mppTexture = mppLoader.createTextureFromFile("assets/suzanne0.ktx");
    auto object1{MppGameObject::createGameObject()};
    object1.model = mppModel;
    object1.texture = mppTexture;
    gameOjects.emplace(object1.getId(), std::move(object1));

    mppTexture = mppLoader.createTextureFromFile("assets/suzanne1.ktx");
    auto object2{MppGameObject::createGameObject()};
    object2.model = mppModel;
    object2.texture = mppTexture;
    gameOjects.emplace(object2.getId(), std::move(object2));

    mppTexture = mppLoader.createTextureFromFile("assets/suzanne2.ktx");
    auto object3{MppGameObject::createGameObject()};
    object3.model = mppModel;
    object3.texture = mppTexture;
    gameOjects.emplace(object3.getId(), std::move(object3));

    mppLoader.initializeTextures();
}


App::App(/* args */)
{
    loadGameObjects();    
    mppLoader.loadShaders();


}

App::~App()
{

}

void App::run()
{
    BasicRenderSystem simpleRenderSystem{mppDevice, mppRenderer, mppLoader };


    uint64_t lastTime{ SDL_GetTicks() };
    bool quit{ false };
    while (!quit) {
        // Wait on fence ********************************
        //czekanie na to aż gpu skończy wykonywać poprzedni commandbuffer
        chk(vkWaitForFences(mppDevice.getLogicalDevice(), 1, &fences[frameIndex], true, UINT64_MAX));
        chk(vkResetFences(mppDevice.getLogicalDevice(), 1, &fences[frameIndex]));

        // Acquire next image ********************************
        chkSwapchain(vkAcquireNextImageKHR(mppDevice.getLogicalDevice(), swapchain, UINT64_MAX, imageAcquiredSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex));

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
        chkSwapchain(vkQueuePresentKHR(queue, &presentInfo));

        // Poll events ********************************

        float elapsedTime{ (SDL_GetTicks()- lastTime) / 1000.0f };
        lastTime = SDL_GetTicks();
        for (SDL_Event event; SDL_PollEvent(&event);)
        {
            switch (event.type)
            {
                //exit loop if aplication is about to close
                case SDL_EVENT_QUIT:
                {
                    quit = true;
                    break;
                }

                //Rotate the selected object with mouse drag
                case SDL_EVENT_MOUSE_MOTION:
                {
                    if(event.button.button == SDL_BUTTON_LEFT) 
                    {
                        objectRotations[shaderData.selected].x -= (float)event.motion.yrel * elapsedTime;
                        objectRotations[shaderData.selected].y += (float)event.motion.xrel * elapsedTime;
                    }
                    break;
                }

                // Zooming with the mouse wheel
                case SDL_EVENT_MOUSE_WHEEL:
                {
                    camPos.z += (float)event.wheel.y * elapsedTime * 10.0f;
                    break;
                }

                // Select active model instance
                case SDL_EVENT_KEY_DOWN:
                {
                    if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
                    {
                        shaderData.selected = (shaderData.selected < 2) ? shaderData.selected + 1 : 0;
                    }
                    if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
                    {
                        shaderData.selected = (shaderData.selected > 0) ? shaderData.selected - 1 : 2;
                    }
                    break;
                }

                //window resize
                case SDL_EVENT_WINDOW_RESIZED:
                {
                    updateSwapchain = true;
                    break;
                }
            
                default:
                    break;
            }
        }

        // Recreate swapchain **********************************

        if (updateSwapchain)
        {
            chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));

            updateSwapchain = false;
            chk(vkDeviceWaitIdle(mppDevice.getLogicalDevice()));//czekanie aż gpu skończy działać
            
            chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mppDevice.getPhysicalDevice(), mppWindow.getSurface(), &mppWindow.getSurfaceCaps()));
            swapchainCI.oldSwapchain = swapchain;
            swapchainCI.imageExtent = {.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y)};
            chk(vkCreateSwapchainKHR(mppDevice.getLogicalDevice(), &swapchainCI, nullptr, &swapchain));

            for(auto i = 0; i< imageCount; i++)
            {
                vkDestroyImageView(mppDevice.getLogicalDevice(), swapchainImageViews[i], nullptr);
            }

            chk(vkGetSwapchainImagesKHR(mppDevice.getLogicalDevice(), swapchain, &imageCount, nullptr));
            swapchainImages.resize(imageCount);
            chk(vkGetSwapchainImagesKHR(mppDevice.getLogicalDevice(), swapchain, &imageCount, swapchainImages.data()));
            
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
                chk(vkCreateImageView(mppDevice.getLogicalDevice(), &viewCI, nullptr, &swapchainImageViews[i]));
            }

            for(auto& semaphore : renderCompleteSemaphores)
            {
                vkDestroySemaphore(mppDevice.getLogicalDevice(), semaphore, nullptr);
            }
            renderCompleteSemaphores.resize(imageCount);
            for(auto& semaphore : renderCompleteSemaphores)
            {
                chk(vkCreateSemaphore(mppDevice.getLogicalDevice(), &semaphoreCI, nullptr, &semaphore));
            }

            vkDestroySwapchainKHR(mppDevice.getLogicalDevice(), swapchainCI.oldSwapchain, nullptr);
            
            vmaDestroyImage(mppDevice.getAllocator(), depthImage, depthImageAllocation);
            vkDestroyImageView(mppDevice.getLogicalDevice(), depthImageView, nullptr);

            depthImageCI.extent = {.width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y), .depth = 1 };
            VmaAllocationCreateInfo allocCI{
                .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO
            };
            chk(vmaCreateImage(mppDevice.getAllocator(), &depthImageCI, &allocCI, &depthImage, &depthImageAllocation, nullptr));
            
            VkImageViewCreateInfo viewCI{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = depthImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = depthFormat,
                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1}
            };
            chk(vkCreateImageView(mppDevice.getLogicalDevice(), &viewCI, nullptr, &depthImageView));
        }
    }

    // CLEANING UP ===========================================

    chk(vkDeviceWaitIdle(mppDevice.getLogicalDevice()));


	vkDestroyDescriptorSetLayout(mppDevice.getLogicalDevice(), descriptorSetLayoutTex, nullptr);
	vkDestroyDescriptorPool(mppDevice.getLogicalDevice(), descriptorPool, nullptr);
	vkDestroyPipelineLayout(mppDevice.getLogicalDevice(), pipelineLayout, nullptr);
	vkDestroyPipeline(mppDevice.getLogicalDevice(), pipeline, nullptr);
	vkDestroySwapchainKHR(mppDevice.getLogicalDevice(), swapchain, nullptr);
	vkDestroySurfaceKHR(mppDevice.getInstance(), mppWindow.getSurface(), nullptr);
	vkDestroyCommandPool(mppDevice.getLogicalDevice(), commandPool, nullptr);
	vkDestroyShaderModule(mppDevice.getLogicalDevice(), shaderModule, nullptr);
	vmaDestroyAllocator(mppDevice.getAllocator());
	SDL_DestroyWindow(window);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_Quit();
	vkDestroyDevice(mppDevice.getLogicalDevice(), nullptr);
	vkDestroyInstance(mppDevice.getInstance(), nullptr);
}




} // namespace mpp


