#include "app.hpp"


namespace mpp
{

void App::loadGameObjects()
{
    std::shared_ptr<MppModel> mppModel = MppModel::createModelFromFile(mppDevice, "assets/suzanne.obj");
    

    std::unique_ptr<MppTexture> mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/suzanne0.ktx");
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));
    
    auto object1{MppGameObject::createGameObject()};
    object1.model = mppModel;
    object1.textureID = 0;
    object1.transform.translation = glm::vec3(-3.0f, 0.0f, 0.0f);
    gameObjects.emplace(object1.getId(), std::move(object1));


    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/suzanne1.ktx");
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    auto object2{MppGameObject::createGameObject()};
    object2.model = mppModel;
    object2.textureID = 1;
    object2.transform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
    gameObjects.emplace(object2.getId(), std::move(object2));


    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/suzanne2.ktx");
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));
    
    auto object3{MppGameObject::createGameObject()};
    object3.model = mppModel;
    object3.textureID = 2;
    object3.transform.translation = glm::vec3(3.0f, 0.0f, 0.0f);
    gameObjects.emplace(object3.getId(), std::move(object3));
}


App::App(/* args */)
{
    loadGameObjects();    
}

App::~App()
{

}

void App::run()
{

    const uint32_t texCount = static_cast<uint32_t>(textureDescriptors.size());

    auto textureLayout = BasicRenderSystem::createTextureSetLayout(mppDevice, texCount);

    globalPool = MppDescriptorPool::Builder(mppDevice)
        .setMaxSets(10)
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texCount)
        .build();

    MppDescriptorWriter(*textureLayout, *globalPool)
        .writeImageArray(0, textureDescriptors.data(), texCount)
        .build(textureDescriptorSet, texCount);

    basicRenderSystem = std::make_unique<BasicRenderSystem>(
        mppDevice, 
        mppRenderer.getSwapChainImageFormat(),
        mppRenderer.getSwapChainDepthFormat(),
        textureLayout->getDescriptorSetLayout()
    );



    ShaderData shaderData{};

    auto viewerObject = MppGameObject::createGameObject();
    viewerObject.transform.translation.z = 6.0f;
    viewerObject.transform.rotation.y = glm::pi<float>();
    CameraController cameraController{};
    SceneInputHandler sceneInputHandler{};
    MppCamera camera{};
    camera.setViewTarget(glm::vec3(-1.0f,-2.0f,2.0f),glm::vec3(0.0f,0.0f,2.5f));
  

    uint64_t lastTime{ SDL_GetTicks() };
    bool quit{ false };
    while (!quit) {

        float elapsedTime{ (SDL_GetTicks()- lastTime) / 1000.0f };
        lastTime = SDL_GetTicks();
        
        

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

                //window resize
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: 
                {
                    mppWindow.setWindowResized(event.window.data1, event.window.data2);
                    break;
                }
            
                default:
                    sceneInputHandler.handleEvent(event, elapsedTime, gameObjects, shaderData.selected);
                    break;
            }
        }

        cameraController.update(elapsedTime, viewerObject);
        camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
        float aspect = mppRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(45.f), aspect, 0.1f, 32.f);
        
        if(auto commandBuffer = mppRenderer.beginFrame())
        {
            //int frameIndex = mppRenderer.getFrameIndex();
            FrameInfo frameInfo{
                //frameIndex, 
                elapsedTime, 
                commandBuffer, 
                //camera, 
                textureDescriptorSet,
                gameObjects,
                mppRenderer.getCurrentShaderDataBufferDeviceAddress()
            };


            // Update shader data ********************************
            //shaderData.projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 32.0f);
            shaderData.projection = camera.getProjection();
            shaderData.view = camera.getView();
            shaderData.cameraPos = glm::vec4(viewerObject.transform.translation, 1.0f);
            //shaderData.view = glm::inverse(viewerObject.transform.mat4());//glm::translate(glm::mat4(1.0f), camPos);
            //shaderData.view = glm::translate(glm::mat4(1.0f), viewerObject.transform.translation);
            for (auto& [id, gameObject] : gameObjects) {
                if (id >= std::size(shaderData.objectData)) break; //TODO: nie jestem pewien czy to nadal potrzebne
                //shaderData.objectData[i].modelMatrix = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i]));
                shaderData.objectData[id].modelMatrix  = gameObject.transform.mat4();
                shaderData.objectData[id].normalMatrix = gameObject.transform.normalMatrix();
            }
            mppRenderer.updateShaderData(shaderData);



            mppRenderer.beginSwapChainRenderPass(commandBuffer);
            
            basicRenderSystem->render(frameInfo);

            mppRenderer.endSwapChainRenderPass(commandBuffer);
            mppRenderer.endFrame(commandBuffer);
        } else {
            SDL_Delay(10);
        }

        //mppRenderer.conditionalRecreateSwapChain();

    }

    chk(vkDeviceWaitIdle(mppDevice.getLogicalDevice()));
}




} // namespace mpp


