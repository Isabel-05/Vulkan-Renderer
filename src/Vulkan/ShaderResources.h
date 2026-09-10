#pragma once
#include "VulkanContext.h"

class ShaderResources
{
public:
	void cleanup(VulkanContext &context);


	//Descriptor Set stuff

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout cameraDSLayout;

	void createDescriptorSetLayouts(VulkanContext& context);
	void createDescriptorPool(VulkanContext& context);

	//Shaders / Pipelines

	VkPipeline BaseShaderPl;
	VkPipelineLayout BaseShaderLayout;

	VkPipeline LineShaderPl;
	VkPipelineLayout LineShaderLayout;

	VkPipeline PointShaderPl;
	VkPipelineLayout PointShaderLayout;

	//TODO
	//VkPipeline OutlineShaderPl;
	//VkPipelineLayout OutlineShaderLayout;

	void createPipelines(VulkanContext& context, const VkFormat& swapchainFormat);


};