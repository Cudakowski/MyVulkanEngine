#include "app.hpp"


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


    ShaderData shaderData{};


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
                    mppRenderer.requestRecreateSwapChain();
                    break;
                }
            
                default:
                    break;
            }
        }

        mppRenderer.conditionalRecreateSwapChain();

    }

    chk(vkDeviceWaitIdle(mppDevice.getLogicalDevice()));
}




} // namespace mpp


