#include "app.hpp"


namespace mpp
{

void App::loadGameObjects()
{
    std::shared_ptr<MppModel> mppModel = MppModel::createModelFromFile(mppDevice,"assets/suzanne.obj");
    std::shared_ptr<MppTexture> mppTexture = mppLoader.createTextureFromFile("assets/suzanne0.ktx");
    auto object1{MppGameObject::createGameObject()};
    object1.model = mppModel;
    object1.texture = mppTexture;
    object1.transform.translation = glm::vec3(-3.0f, 0.0f, 0.0f);
    gameObjects.emplace(object1.getId(), std::move(object1));

    mppTexture = mppLoader.createTextureFromFile("assets/suzanne1.ktx");
    auto object2{MppGameObject::createGameObject()};
    object2.model = mppModel;
    object2.texture = mppTexture;
    object2.transform.translation = glm::vec3(0.0f, 0.0f, 0.0f);
    gameObjects.emplace(object2.getId(), std::move(object2));

    mppTexture = mppLoader.createTextureFromFile("assets/suzanne2.ktx");
    auto object3{MppGameObject::createGameObject()};
    object3.model = mppModel;
    object3.texture = mppTexture;
    object3.transform.translation = glm::vec3(3.0f, 0.0f, 0.0f);
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
                mppLoader.getDescriptorSetTex(),
                gameObjects
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
            
            basicRenderSystem.render(frameInfo);

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


