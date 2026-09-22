#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

class IdRenderpass 
{
public:
	void createResources(VulkanContext& context, VkDescriptorSetLayout cameraDs, VkExtent2D inExtent);
	void resize(VulkanContext& context, VkExtent2D inExtent);
	void cleanup(VulkanContext& context);
	
	VkPipeline idEditPipeline;
	VkPipelineLayout idEditPipelineLayout;

	VkPipeline idObjectPipeline;
	VkPipelineLayout idObjectPipelineLayout;

	VkImage texture;
	VkImageView textureView;
	VkDeviceMemory textureMemory;

	VkBuffer readbackBuffer;
	VkDeviceMemory readbackBufferMemory;

	VkExtent2D extent;

	const uint32_t pickRadius = 8;
	const uint32_t boxSize = (pickRadius * 2) + 1;
};