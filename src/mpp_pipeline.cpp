#include "mpp_pipeline.hpp"

namespace mpp
{

MppPipeline::MppPipeline(MppDevice& device, MppLoader& loader, MppRenderer& renderer): mppDevice{device}, mppLoader{loader}, mppRenderer{renderer}
{
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
        .pSetLayouts = &mppLoader.getDescriptorSetLayoutTex(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };
    chk(vkCreatePipelineLayout(mppDevice.getLogicalDevice(), &pipelineLayoutCI, nullptr, &pipelineLayout));

    // layout of the vertex data
    VkVertexInputBindingDescription vertexBinding{
        .binding = 0,
        .stride = sizeof(MppModel::Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    std::vector<VkVertexInputAttributeDescription> vertexAttributes{
        {.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT },
        {.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset  = offsetof(MppModel::Vertex, normal) },
        {.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset  = offsetof(MppModel::Vertex, uv) },
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

    // podłaczenie shaderów do pipeline'a
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages{
        {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_VERTEX_BIT,
         .module = mppLoader.getShaderModule(), .pName = "main" },
        {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
         .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
         .module = mppLoader.getShaderModule(), .pName = "main" }
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
        .pColorAttachmentFormats = &(renderer.getSwapChainImageFormat()),
        .depthAttachmentFormat =renderer.getSwapChainDepthFormat()
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
    chk(vkCreateGraphicsPipelines(mppDevice.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));

}

void MppPipeline::bind(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}


} // namespace mpp
