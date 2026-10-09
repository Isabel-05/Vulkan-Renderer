#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include <glm/glm.hpp>

struct IdPushConstants
{
	glm::mat4 model;
	uint32_t id;
};

class IdRenderpass 
{
public:
	void createResources(VulkanContext& context, VkDescriptorSetLayout cameraDs, VkDescriptorSetLayout outlineDS, VkDescriptorPool descriptorPool, VkExtent2D inExtent, uint32_t maxFramesInFlight);
	void updateDescriptorSets(VulkanContext& context);
	void resize(VulkanContext& context, VkExtent2D inExtent);
	void cleanup(VulkanContext& context);
	
	VkPipeline idEditPipeline;
	VkPipelineLayout idEditPipelineLayout;

	VkPipeline idObjectPipeline;
	VkPipelineLayout idObjectPipelineLayout;

	std::vector<VkImage> idTextures;
	std::vector<VkImageView> idTextureViews;
	std::vector<VkDeviceMemory> idTextureMemories;

	VkBuffer readbackBuffer;
	VkDeviceMemory readbackBufferMemory;

	VkSampler outlineSampler;
	std::vector<VkDescriptorSet> outlineDescriptorSets;

	VkExtent2D extent;

	const uint32_t pickRadius = 8;
	const uint32_t boxSize = (pickRadius * 2) + 1;
};