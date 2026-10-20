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
    gameObjects.emplace(object1.getId(), std::move(object1));

    mppTexture = mppLoader.createTextureFromFile("assets/suzanne1.ktx");
    auto object2{MppGameObject::createGameObject()};
    object2.model = mppModel;
    object2.texture = mppTexture;
    gameObjects.emplace(object2.getId(), std::move(object2));

    mppTexture = mppLoader.createTextureFromFile("assets/suzanne2.ktx");
    auto object3{MppGameObject::createGameObject()};
    object3.model = mppModel;
    object3.texture = mppTexture;
    gameObjects.emplace(object3.getId(), std::move(object3));

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
    BasicRenderSystem basicRenderSystem{mppDevice, mppRenderer, mppLoader };


    uint64_t lastTime{ SDL_GetTicks() };
    bool quit{ false };
    while (!quit) {

        float elapsedTime{ (SDL_GetTicks()- lastTime) / 1000.0f };
        lastTime = SDL_GetTicks();
        
        if(auto commandBuffer = mppRenderer.beginFrame())
        {
            //int frameIndex = mppRenderer.getFrameIndex();
            FrameInfo frameInfo{
                //frameIndex, 
                elapsedTime, 
                commandBuffer, 
                //camera, 
                mppLoader.getDescriptorSetTex(),
                gameObjects
            };

            // Update shader data ********************************
            ShaderData shaderData{};
            shaderData.projection = glm::perspective(glm::radians(45.0f), (float)mppWindow.getWindowSize().x / (float)mppWindow.getWindowSize().y, 0.1f, 32.0f);
            shaderData.view = glm::translate(glm::mat4(1.0f), camPos);
            for (auto i = 0; i<3; i++) {
                auto instancePos = glm::vec3((float)(i-1)*3.0f, 0.0f, 0.0f);
                shaderData.model[i] = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i]));
            }
            mppRenderer.updateShaderData(shaderData);

            mppRenderer.beginSwapChainRenderPass(commandBuffer);
            
            basicRenderSystem.render(frameInfo);

            mppRenderer.endSwapChainRenderPass(commandBuffer);
            mppRenderer.endFrame(commandBuffer);
        }

        // Poll events ********************************

        
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


