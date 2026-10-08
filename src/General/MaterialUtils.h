#pragma once
#include "VulkanContext.h"

namespace MaterialUtils
{
	void createPipeline(VulkanContext& context, std::string vertShaderPath, std::string fragShaderPath, VkDescriptorSetLayout cameraDSLayout, VkBool32 blendEnable,
		VkBool32 depthWriteEnable, VkBool32 depthTestEnable, VkPrimitiveTopology topology, VkPolygonMode polygonMode, const VkFormat& outputFormat, uint32_t pushconstantSize, VkSampleCountFlagBits samples,
		VkVertexInputBindingDescription bindingDesc, const std::vector<VkVertexInputAttributeDescription>& attributeDescs,
		VkPipeline& outPipeline, VkPipelineLayout& outPipelineLayout);
}