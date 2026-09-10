#pragma once
#include "VulkanContext.h"

namespace MaterialUtils
{
	void createPipeline(VulkanContext& context, std::string vertShaderPath, std::string fragShaderPath, VkDescriptorSetLayout cameraDSLayout,
		VkPolygonMode polygonMode, const VkFormat& swapchainFormat, VkPipeline& outPipeline, VkPipelineLayout& outPipelineLayout);
}