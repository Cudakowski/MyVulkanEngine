#include "app.hpp"

#include "mpp_block_chunk.hpp"
#include "player_controller.hpp"

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

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/unbreak.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/dirt.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/stone.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/sand.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/grass.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/brick.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/wood.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/leaf.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));

    mppTexture = MppTexture::createTextureFromFile(mppDevice, "assets/blocks/planks.ktx", true);
    textureDescriptors.push_back(mppTexture->getDescriptorInfo());
    textures.push_back(std::move(mppTexture));
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

    blockRenderSystem = std::make_unique<Block::BlockRenderSystem>(
        mppDevice, 
        mppRenderer.getSwapChainImageFormat(),
        mppRenderer.getSwapChainDepthFormat(),
        textureLayout->getDescriptorSetLayout()
    );

    initImGui();

    // Game data
    ShaderData shaderData{};

    uint32_t cameraIndex{};
    auto viewerObject = MppGameObject::createGameObject();
    viewerObject.transform.translation.z = 6.0f;
    viewerObject.transform.rotation.y = glm::pi<float>();
    auto playerObject = MppGameObject::createGameObject();
    constexpr float gravity{15.f};
    glm::vec3 playerVelocity{};
    Block::BlockID currentBlock{Block::BlockID::BRICK};
    CameraController cameraController{};
    SceneInputHandler sceneInputHandler{};
    PlayerController playerController{};
    MppCamera camera{};
    camera.setViewTarget(glm::vec3(-1.0f,-2.0f,2.0f),glm::vec3(0.0f,0.0f,2.5f));

    std::vector<std::unique_ptr<mpp::Block::MppBlockChunk>> chunks;
    chunks.push_back(std::make_unique<mpp::Block::MppBlockChunk>(mppDevice));

    chunks[0]->calculateMesh();
    playerObject.transform.translation = chunks[0]->determinePlayerStartingPosition();

    uint64_t lastTime{ SDL_GetTicksNS() };
    bool quit{ false };
    while (!quit) {

        float elapsedTime{ static_cast<float>(SDL_GetTicksNS()- lastTime) / 1'000'000'000.0f };
        lastTime = SDL_GetTicksNS();

        elapsedTime = std::min(elapsedTime, 0.1f);//ograniczenie
        

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
                    bool mouseHandled    = io.WantCaptureMouse    && (event.type >= SDL_EVENT_MOUSE_MOTION && event.type <= SDL_EVENT_MOUSE_WHEEL);
                    bool keyboardHandled = io.WantCaptureKeyboard && (event.type >= SDL_EVENT_KEY_DOWN     && event.type <= SDL_EVENT_KEY_UP);

                    if (!mouseHandled && !keyboardHandled) {
                        //sceneInputHandler.handleEvent(event, elapsedTime, gameObjects, shaderData.selected, cameraIndex, mppWindow, quit, chunks);
                        auto& selectedId = shaderData.selected;
                        float mouseSensitivity{0.01f};
                        switch (event.type) {

                            //Rotate the selected object with mouse drag
                            case SDL_EVENT_MOUSE_MOTION:
                            {
                                if(cameraIndex ==0)
                                if(event.motion.state & SDL_BUTTON_LMASK) //event.button.button == SDL_BUTTON_LEFT
                                {
                                    auto it = gameObjects.find(selectedId);
                                    if (it != gameObjects.end()) {
                                        auto& obj = it->second;
                                        // obj.transform.rotation.x -= (float)event.motion.yrel * dt * mouseSensitivity;
                                        // obj.transform.rotation.y -= (float)event.motion.xrel * dt * mouseSensitivity;
                                        obj.transform.rotation.x -= static_cast<float>(event.motion.yrel) * mouseSensitivity;
                                        obj.transform.rotation.y -= static_cast<float>(event.motion.xrel) * mouseSensitivity;

                                    }
                                }
                                break;
                            }

                            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                            {
                                if(cameraIndex ==1)
                                if(event.button.button == SDL_BUTTON_LEFT) //event.button.button == SDL_BUTTON_LEFT
                                {
                                    chunks[0]->destroyBlock(playerObject.transform.translation, playerObject.transform.rotation);
                                }
                                if(cameraIndex ==1)
                                if(event.button.button == SDL_BUTTON_RIGHT) //event.button.button == SDL_BUTTON_RIGHT
                                {
                                    chunks[0]->placeBlock(playerObject.transform.translation, playerObject.transform.rotation, currentBlock);
                                }
                                break;
                            }
                            case SDL_EVENT_MOUSE_WHEEL:
                            {
                                if(cameraIndex ==1)
                                if (event.wheel.y > 0.0f) 
                                {
                                    auto next = static_cast<Block::BlockType>(currentBlock) + 1;
                                    if (next > static_cast<Block::BlockType>(Block::BlockID::PLANKS)) {
                                        next = static_cast<Block::BlockType>(Block::BlockID::DIRT);
                                    }
                                    currentBlock = static_cast<Block::BlockID>(next);
                                }
                                else if (event.wheel.y < 0.0f) 
                                {
                                    auto prev = static_cast<Block::BlockType>(currentBlock) - 1;
                                    if (prev < static_cast<Block::BlockType>(Block::BlockID::DIRT)) {
                                        prev = static_cast<Block::BlockType>(Block::BlockID::PLANKS);
                                    }
                                    currentBlock = static_cast<Block::BlockID>(prev);
                                }
                                break;
                            }


                            // Select active model instance
                            case SDL_EVENT_KEY_DOWN:
                            {
                                if (event.key.key == SDLK_ESCAPE)
                                {
                                    quit = true;
                                }
                                if (event.key.key == SDLK_PLUS || event.key.key == SDLK_KP_PLUS)
                                {
                                    selectedId = (selectedId < 2) ? selectedId + 1 : 0;
                                }
                                if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS)
                                {
                                    selectedId = (selectedId > 0) ? selectedId - 1 : 2;
                                }
                                if (event.key.key == SDLK_0 )
                                {
                                    cameraIndex = 0;
                                    mppWindow.setRelativeMouseMode(false);
                                }
                                if (event.key.key == SDLK_1 )
                                {
                                    cameraIndex = 1;
                                    mppWindow.setRelativeMouseMode(true);
                                }
                                break;
                            }

                            default:
                                break;
                        }
                    }
                    break;
            }
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if(cameraIndex ==0)
        {
            ImGui::Begin("Engine Diagnostics");
            ImGui::Text("FPS: %.1f (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("Rendered objects: %zu", gameObjects.size());
            ImGui::Separator();
            ImGui::DragFloat3("Camera Pos", &viewerObject.transform.translation.x, 0.1f);
            ImGui::End();
        }
        if(cameraIndex ==1)
        {
            ImGui::Begin("Majkarp Diagnostics");
            ImGui::Text("FPS: %.1f (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImGui::Text("Current block: %s", Block::toString(currentBlock).data());
            ImGui::End();
        }


        ImGui::Render();

        
        switch (cameraIndex)
        {
        case 0:
            cameraController.update(elapsedTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
            break;
        case 1:
            playerVelocity.y += gravity * elapsedTime;
            chunks[0]->checkCollision(playerObject.transform.translation, playerVelocity, elapsedTime);
            playerController.update(elapsedTime,playerObject, playerVelocity);
            camera.setViewYXZ(playerObject.transform.translation+glm::vec3{0.f, -1.8f, 0.f}, playerObject.transform.rotation);
            break;
        
        default:
            cameraController.update(elapsedTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);
            break;
        }


        
        float aspect = mppRenderer.getAspectRatio();
        camera.setPerspectiveProjection(glm::radians(45.f), aspect, 0.1f, 64.f);
        
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
            
            if(cameraIndex == 1)
            {
                for (auto& chunk : chunks) {
                    if (chunk->isDirty()) {
                        chunk->calculateMesh();
                    }
                }
                blockRenderSystem->render(frameInfo, chunks);
            }
            else
            {
                basicRenderSystem->render(frameInfo);
            }
            

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


