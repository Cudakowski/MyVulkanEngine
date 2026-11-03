#include "mpp_pipeline.hpp"

namespace mpp
{
MppPipeline::~MppPipeline()
{
    vkDestroyPipeline(mppDevice.getLogicalDevice(), pipeline, nullptr);
}

MppPipeline::MppPipeline(MppDevice& device, MppLoader& loader, MppRenderer& renderer,const PipelineConfigInfo& configInfo, const VkPipelineLayout &pipelineLayout): mppDevice{device}, mppLoader{loader}, mppRenderer{renderer}
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

    // podłaczenie shaderów do pipeline'a
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{//////////////
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = mppLoader.getShaderModule(), 
            .pName = "main",
            .pSpecializationInfo = nullptr, },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = mppLoader.getShaderModule(), 
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
        .pColorAttachmentFormats = &(mppRenderer.getSwapChainImageFormat()),
        .depthAttachmentFormat =mppRenderer.getSwapChainDepthFormat()
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

    VkPipelineColorBlendStateCreateInfo colorBlendState{
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
