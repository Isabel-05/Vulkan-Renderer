#pragma once
#include "VulkanContext.h"

class GraphicsPipeline
{
public:
	GraphicsPipeline() = default;
	~GraphicsPipeline() = default;

	void create(VulkanContext& context, VkRenderPass renderPass, VkDescriptorSetLayout& descriptorSetLayout);
	void cleanup(VulkanContext& context);

	VkPipeline handle;
	VkPipelineLayout pipelineLayout;

private:
	VkShaderModule createShaderModule(VulkanContext& context, const std::vector<char>& code);
	std::vector<char> readFile(const std::string& filename);
};

class RenderPass
{
public:

	RenderPass() = default;
	~RenderPass() = default;

	void create(VulkanContext& context, VkFormat imageFormat);
	void cleanup(VulkanContext& context);

	VkRenderPass handle;
};