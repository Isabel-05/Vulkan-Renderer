#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

class IdRenderpass 
{
public:
	void createResources(VulkanContext& context, VkDescriptorSetLayout cameraDs, VkExtent2D inExtent);
	void resize(VulkanContext& context, VkExtent2D inExtent);
	void cleanup(VulkanContext& context);

	uint32_t pickObject(VulkanContext& context, CommandPool& cmdPool, uint32_t x, uint32_t y);
	uint32_t pickVertex(VulkanContext& context, CommandPool& cmdPool, uint32_t x, uint32_t y);

private:

	const uint32_t boxSize = 8;

	VkImage texture;
	VkImageView textureView;
	VkDeviceMemory textureMemory;

	VkBuffer readbackBuffer;
	VkDeviceMemory readbackBufferMemory;

	VkPipeline idEditPipeline;
	VkPipelineLayout idEditPipelineLayout;

	VkPipeline idObjectPipeline;
	VkPipelineLayout idObjectPipelineLayout;

	VkExtent2D extent;
};