#pragma once
#include "VulkanContext.h"
#include <glm/glm.hpp>

struct OutlinePushConstants
{
	uint32_t   selectedId;
	int32_t    thickness;
	glm::ivec2 texSize;
};

struct GridPushConstants
{
	glm::mat4 invViewProj;
	glm::vec3 cameraPos;
	float     cellSize;
	float     fadeNear;
	float     fadeFar;
};

class ShaderResources
{
public:
	void cleanup(VulkanContext &context);


	//Descriptor Set stuff

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout cameraDSLayout;
	VkDescriptorSetLayout outlineDSLayout;

	void createDescriptorSetLayouts(VulkanContext& context);
	void createDescriptorPool(VulkanContext& context);

	//Shaders / Pipelines

	VkPipeline BaseShaderPl;
	VkPipelineLayout BaseShaderLayout;

	VkPipeline LineShaderPl;
	VkPipelineLayout LineShaderLayout;

	VkPipeline PointShaderPl;
	VkPipelineLayout PointShaderLayout;

	VkPipeline OutlineShaderPl;
	VkPipelineLayout OutlineShaderLayout;

	VkPipeline GridShaderPl;
	VkPipelineLayout GridShaderLayout;

	void createPipelines(VulkanContext& context, const VkFormat& swapchainFormat);

	void createOutlinePipeline(VulkanContext& context, const VkFormat& swapchainFormat);
	void createGridPipeline(VulkanContext& context, const VkFormat& swapchainFormat, VkSampleCountFlagBits samples);
};