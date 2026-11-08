#pragma once


#include "mpp_device.hpp"
#include "mpp_model.hpp"
#include "mpp_renderer.hpp"

#include "mpp_checks.hpp"

#include <vulkan/vulkan.h>
#include <volk/volk.h>

#include <cstring>

namespace mpp
{

struct PipelineConfigInfo {
  PipelineConfigInfo() = default;
  PipelineConfigInfo(const PipelineConfigInfo&)=delete;
  PipelineConfigInfo& operator=(const PipelineConfigInfo&)=delete;

  std::vector<VkVertexInputBindingDescription> bindingDescriptions{}; 
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions{}; 
  VkPipelineViewportStateCreateInfo viewportInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo multisampleInfo;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineColorBlendStateCreateInfo colorBlendInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  std::vector<VkDynamicState> dynamicStateEnables;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo;
};



class MppPipeline 
{
private:
    MppDevice& mppDevice;

    VkPipeline pipeline{ VK_NULL_HANDLE };
    VkShaderModule shaderModule{};//TODO: Podzielić to kiedy będzie to potrzebne
    // VkShaderModule vertShaderModule{};
    // VkShaderModule fragShaderModule{};
    void createShaderModule(const std::vector<uint32_t>& data, VkShaderModule& shaderModule);
public:

    MppPipeline(MppDevice& device,const PipelineConfigInfo& configInfo,const VkPipelineLayout &pipelineLayout, const VkFormat& swapChainImageFormat, VkFormat swapChainDepthFormat);
    ~MppPipeline();

    MppPipeline(const MppPipeline&) = delete;
    MppPipeline& operator=(const MppPipeline&) = delete;
    

    void bind(VkCommandBuffer commandBuffer);
    static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
    static void enableAlphaBlending(PipelineConfigInfo& configInfo); //TODO
};




} // namespace mpp


