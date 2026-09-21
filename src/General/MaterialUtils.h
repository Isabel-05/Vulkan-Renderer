#pragma once
#include "VulkanContext.h"

namespace MaterialUtils
{
	void createPipeline(VulkanContext& context, std::string vertShaderPath, std::string fragShaderPath, VkDescriptorSetLayout cameraDSLayout,
		VkPolygonMode polygonMode, const VkFormat& swapchainFormat, VkPipeline& outPipeline, VkPipelineLayout& outPipelineLayout);

	void createIdPipeline(VulkanContext& context, std::string vertShaderPath, std::string fragShaderPath, VkDescriptorSetLayout cameraDSLayout, 
		VkPrimitiveTopology topology, VkPolygonMode polygonMode, VkPipeline& outPipeline, VkPipelineLayout& outPipelineLayout);
}