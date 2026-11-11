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

void App::checkAssets() const
{
    // Make sure asset folder is present from the current working directory
    if (!std::filesystem::is_directory("assets")) {
        throw std::runtime_error("Could not locate assets folder from current working directory\n");
    }
}


App::App(/* args */)
{
    loadGameObjects();    
}

App::~App()
{
    cleanupImGui();
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

    initImGui();

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
            ImGui_ImplSDL3_ProcessEvent(&event);

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
                    //blokowanie zdarzeń kiedy jest interakcja z imgui
                    ImGuiIO& io = ImGui::GetIO();
                    bool mouseHandled = io.WantCaptureMouse && 
                        (event.type >= SDL_EVENT_MOUSE_MOTION && event.type <= SDL_EVENT_MOUSE_WHEEL);
                    bool keyboardHandled = io.WantCaptureKeyboard && 
                        (event.type >= SDL_EVENT_KEY_DOWN && event.type <= SDL_EVENT_KEY_UP);

                    if (!mouseHandled && !keyboardHandled) {
                        sceneInputHandler.handleEvent(event, elapsedTime, gameObjects, shaderData.selected);
                    }
                    break;
            }
        }

        // 1. Start nowej klatki ImGui
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 2. Przykładowe okienko diagnostyczne
        ImGui::Begin("Engine Diagnostics");
        ImGui::Text("FPS: %.1f (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("Rendered objects: %zu", gameObjects.size());
        ImGui::Separator();
        ImGui::DragFloat3("Camera Pos", &viewerObject.transform.translation.x, 0.1f);
        ImGui::End();

        // 3. Zakończenie definicji UI i wygenerowanie geometrii wierzchołków
        ImGui::Render();

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
            shaderData.projection = camera.getProjection();
            shaderData.view = camera.getView();
            shaderData.cameraPos = glm::vec4(viewerObject.transform.translation, 1.0f);
            for (auto& [id, gameObject] : gameObjects) {
                if (id >= std::size(shaderData.objectData)) break; //TODO: nie jestem pewien czy to nadal potrzebne
                //shaderData.objectData[i].modelMatrix = glm::translate(glm::mat4(1.0f), instancePos) * glm::mat4_cast(glm::quat(objectRotations[i]));
                shaderData.objectData[id].modelMatrix  = gameObject.transform.mat4();
                shaderData.objectData[id].normalMatrix = gameObject.transform.normalMatrix();
            }
            mppRenderer.updateShaderData(shaderData);



            mppRenderer.beginSwapChainRenderPass(commandBuffer);
            
            basicRenderSystem->render(frameInfo);

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

            mppRenderer.endSwapChainRenderPass(commandBuffer);
            mppRenderer.endFrame(commandBuffer);
        } else {
            SDL_Delay(10);
        }

        //mppRenderer.conditionalRecreateSwapChain();

    }

    chk(vkDeviceWaitIdle(mppDevice.getLogicalDevice()));
}


void App::initImGui()
{


    // 2. Inicjalizacja kontekstu ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Gałąź docking

    ImGui::StyleColorsDark();

    // 3. Backend platformy (SDL3)
    ImGui_ImplSDL3_InitForVulkan(mppWindow.getSDLWindow());

    VkFormat colorFormat = mppRenderer.getSwapChainImageFormat();
    VkFormat depthFormat = mppRenderer.getSwapChainDepthFormat();

    // 1. Konfiguracja Dynamic Rendering dla głównego potoku ImGui
    VkPipelineRenderingCreateInfoKHR renderingCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = depthFormat
    };

    ImGui_ImplVulkan_LoadFunctions(
        VK_API_VERSION_1_3,
        [](const char* function_name, void* user_data) -> PFN_vkVoidFunction {
            return vkGetInstanceProcAddr(reinterpret_cast<VkInstance>(user_data), function_name);
        },
        mppDevice.getInstance()
    );
    
    // 2. Wypełnienie nowej struktury ImGui_ImplVulkan_InitInfo
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_3; // Wskazujemy Vulkan 1.3
    initInfo.Instance = mppDevice.getInstance();
    initInfo.PhysicalDevice = mppDevice.getPhysicalDevice();
    initInfo.Device = mppDevice.getLogicalDevice();
    initInfo.QueueFamily = mppDevice.findPhysicalQueueFamilies().graphicsFamily;
    initInfo.Queue = mppDevice.getGraphicsQueue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = VK_NULL_HANDLE;
    initInfo.DescriptorPoolSize = 100;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 3;

    // Flaga Dynamic Rendering
    initInfo.UseDynamicRendering = true;

    // PipelineInfoMain - to tutaj trafiły przeniesione pola:
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingCreateInfo;

    ImGui_ImplVulkan_Init(&initInfo);
}

void App::cleanupImGui()
{
    vkDeviceWaitIdle(mppDevice.getLogicalDevice());
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

} // namespace mpp


