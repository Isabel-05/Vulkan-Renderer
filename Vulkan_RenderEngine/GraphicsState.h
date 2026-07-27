#pragma once
#include "VulkanContext.h"

class GraphicsPipeline
{
public:
	GraphicsPipeline() = default;
	~GraphicsPipeline() = default;

	void create(VulkanContext& context, VkFormat swapchainFormat, VkDescriptorSetLayout& descriptorSetLayout);
	void cleanup(VulkanContext& context);

	VkPipeline handle;
	VkPipelineLayout pipelineLayout;
};