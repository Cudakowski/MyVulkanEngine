#include "mpp_pipeline.hpp"

#include "slang/slang.h"
#include "slang/slang-com-ptr.h"
#include <filesystem>
#include <fstream>

namespace mpp
{

void loadShaders(std::vector<uint32_t> &spirvData)//TODO: Lepiej zaprojektować
{
    // LOADING SHADERS =====================================================
    
    // kumpilowanie slanga do SPIR-V w runtimie :D (TODO:: tylko na czas developement'u i do edytora(ryyko bezpieczeństwa))
    // SPIR-V to taki pośredniczący język shaderów który vulkan umie czytać. 
    // GLSL też do niego najpierw sie kompiluje.
    Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;

    const std::filesystem::path sourcePath = "assets/shader.slang";
    const std::filesystem::path cachePath  = "assets/shader.spv";

    
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

    
}

void MppPipeline::createShaderModule(const std::vector<uint32_t>& data, VkShaderModule& shaderModule)
{
    // tworzenie modułu shadera (kontener na skompilowany SPRI-V)
    VkShaderModuleCreateInfo shaderModuleCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = data.size() * sizeof(uint32_t),
        .pCode = data.data()
    };

    if (vkCreateShaderModule(mppDevice.getLogicalDevice(), &shaderModuleCI, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module");
    }
    /* To do nowszej wersji vulkana
    The VK_KHR_maintenance5 extension, which became core with 
    Vulkan 1.4, deprecated shader modules. It allows direct 
    passing of VkShaderModuleCreateInfo to the pipeline's 
    shader stage create info.
    */

    // THE SHADER =============================================

    // (pacz shader w assetach)
}

MppPipeline::~MppPipeline()
{
    vkDestroyShaderModule(mppDevice.getLogicalDevice(), shaderModule, nullptr);
    vkDestroyPipeline(mppDevice.getLogicalDevice(), pipeline, nullptr);
}

MppPipeline::MppPipeline(MppDevice& device,const PipelineConfigInfo& configInfo, const VkPipelineLayout &pipelineLayout, const VkFormat& swapChainImageFormat, VkFormat swapChainDepthFormat): mppDevice{device}
{
    assert(pipelineLayout != VK_NULL_HANDLE && "cannot create graphics pipeline:: no pipelineLayout provided in configInfo");

    
    // // layout of the vertex data
    // VkVertexInputBindingDescription vertexBinding{
    //     .binding = 0,
    //     .stride = sizeof(MppModel::Vertex),
    //     .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    // };

    // std::vector<VkVertexInputAttributeDescription> vertexAttributes{
    //     {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT },
    //     {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset  = offsetof(MppModel::Vertex, normal) },
    //     {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset  = offsetof(MppModel::Vertex, uv) },
    // };

    /*
    Another option for accessing vertices in the shader is buffer 
    device address. That way we would skip the traditional vertex 
    attributes and manually fetch them in the shader using pointers. That's called "vertex pulling". On some devices that can be slower though, so we stick with the traditional way.
    */

    VkPipelineVertexInputStateCreateInfo vertexInputState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount =  static_cast<uint32_t>(configInfo.bindingDescriptions.size()),
        .pVertexBindingDescriptions = configInfo.bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(configInfo.attributeDescriptions.size()),
        .pVertexAttributeDescriptions = configInfo.attributeDescriptions.data()
    };

    // //primitives
    // VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    //     .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    // };
    std::vector<uint32_t> spirvData{};
    loadShaders(spirvData);
    createShaderModule(spirvData,shaderModule);

    

    // podłaczenie shaderów do pipeline'a
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{//////////////
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shaderModule, 
            .pName = "main",
            .pSpecializationInfo = nullptr, },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shaderModule, 
            .pName = "main",
            .pSpecializationInfo = nullptr, }
    };  

    /*
    If you'd wanted to use different shaders 
    (or shader combinations), you'd have to create multiple 
    pipelines. The VK_EXT_shader_objects makes those shader 
    stages into separate objects and adds more flexibility 
    to this part of the API.
    */

    // //viewport (dynamic - nie trzeba recreate pipeline kiedy się zmienią)
    // VkPipelineViewportStateCreateInfo viewportState{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    //     .viewportCount =1,
    //     .scissorCount =1
    // };
    // std::vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    // VkPipelineDynamicStateCreateInfo dynamicState{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    //     .dynamicStateCount = 2,
    //     .pDynamicStates = dynamicStates.data()
    // };

    // //depth settings
    // VkPipelineDepthStencilStateCreateInfo depthStencilState{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    //     .depthTestEnable = VK_TRUE,
    //     .depthWriteEnable = VK_TRUE,
    //     .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    // };

    //dynamic rendering zamiast render pass objects :DDD
    VkPipelineRenderingCreateInfo renderingCI{///////////////////////////////////////
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount =1,
        .pColorAttachmentFormats = &swapChainImageFormat,
        .depthAttachmentFormat =swapChainDepthFormat
    };

    // //nie będzie to używane ale trzeba to uspecyfikować jakimiś domyślnymi wartościami
    // VkPipelineColorBlendAttachmentState blendAttachment{
    //     .colorWriteMask = 0xF
    // };
    // VkPipelineColorBlendStateCreateInfo colorBlendState{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    //     .attachmentCount = 1,
    //     .pAttachments = &blendAttachment
    // };
    // VkPipelineRasterizationStateCreateInfo rasterizationState{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    //     .lineWidth = 1.0f
    // };
    // VkPipelineMultisampleStateCreateInfo multisampleState{
    //     .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    //     .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    // };

    //creating pipeline :DDD
    //dynamic rendering => nie podaje się tutaj render pass'a
    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCI,//zamiast render pass'a
        .stageCount = 2,
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &configInfo.inputAssemblyInfo,
        .pViewportState = &configInfo.viewportInfo,
        .pRasterizationState = &configInfo.rasterizationInfo,
        .pMultisampleState = &configInfo.multisampleInfo,
        .pDepthStencilState = &configInfo.depthStencilInfo,
        .pColorBlendState = &configInfo.colorBlendInfo,
        .pDynamicState = &configInfo.dynamicStateInfo,
        .layout = pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE, //nie wiem co to robi
        .basePipelineIndex = -1,  //nie wiem co to robi
        // Tych nie ma bo jest dynamiczne renderowanie:
        //.subpass = configInfo.subpass,
        //.renderPass = configInfo.renderPass,
    };
    chk(vkCreateGraphicsPipelines(mppDevice.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

}

void MppPipeline::bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void MppPipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo)
{
    //primitives
    configInfo.inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    //viewport (dynamic - nie trzeba recreate pipeline kiedy się zmienią)
    configInfo.viewportInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount =1,
        .pViewports = nullptr,
        .scissorCount =1,
        .pScissors = nullptr
    };

    configInfo.rasterizationInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE, // :DDD
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,    //optional
        .depthBiasClamp = 0.0f,             //optional
        .depthBiasSlopeFactor = 0.0f,       //optional
        .lineWidth = 1.0f, //ciekawe co to robi xd
    };

    configInfo.multisampleInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,           //optional
        .pSampleMask = nullptr,             //optional
        .alphaToCoverageEnable = VK_FALSE,  //optional
        .alphaToOneEnable = VK_FALSE,       //optional
    };

    //nie będzie to używane ale trzeba to uspecyfikować jakimiś domyślnymi wartościami
    configInfo.colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,     //optional
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,    //optional
        .colorBlendOp = VK_BLEND_OP_ADD,                //optional
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,     //optional
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,    //optional
        .alphaBlendOp = VK_BLEND_OP_ADD,                //optional
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    configInfo.colorBlendInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp =  VK_LOGIC_OP_COPY,   //optional
        .attachmentCount = 1,
        .pAttachments = &configInfo.colorBlendAttachment,
        .blendConstants={0.0f,0.0f,0.0f,0.0f},        //optional
    };

    //depth settings
    configInfo.depthStencilInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable =  VK_FALSE,
        .front = {}, //optional
        .back = {},  //optional
        .minDepthBounds = 0.0f, //optional
        .maxDepthBounds = 1.0f, //optional
    };


    configInfo.dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    configInfo.dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .flags=0,
        .dynamicStateCount = static_cast<uint32_t>(configInfo.dynamicStateEnables.size()),
        .pDynamicStates = configInfo.dynamicStateEnables.data(),
    };


    //configInfo.bindingDescriptions = MppModel::Vertex::getBindingDescriptions(); // TODO: nie wiem, chyba doprze jest wprowadzić te funkcje
    //configInfo.attributeDescriptions = MppModel::Vertex::getAttributeDescriptions();
    
    // layout of the vertex data
    configInfo.bindingDescriptions = {
        {
            .binding = 0,
            .stride = sizeof(MppModel::Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        }
    };

    configInfo.attributeDescriptions = {
        {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT },
        {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset  = offsetof(MppModel::Vertex, normal) },
        {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset  = offsetof(MppModel::Vertex, uv) },
    };

}


void MppPipeline::enableAlphaBlending(PipelineConfigInfo& configInfo){
    configInfo.colorBlendAttachment.blendEnable = VK_TRUE;

    configInfo.colorBlendAttachment.colorWriteMask = 
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}


} // namespace mpp
