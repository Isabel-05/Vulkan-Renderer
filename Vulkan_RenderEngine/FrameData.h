#pragma once
#include "VulkanContext.h"
#include "BufferUtils.h"
#include <glm/glm.hpp>


struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class FrameData
{
public:
	FrameData() = default;
	~FrameData() = default;

	void cleanup(VulkanContext& context, size_t imageCount);

	uint32_t maxFramesInFlight = 2;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkDeviceMemory> commandBuffersMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkDescriptorSet> descriptorSets;


	void createUniformBuffers(VulkanContext& context);
	void createCommandBuffers(VulkanContext& context, CommandPool& cmdPool);
	void createSyncObjects(VulkanContext& context, size_t imageCount);

	void createDescriptorSetLayout(VulkanContext& context);
	void createDescriptorPool(VulkanContext& context);
	void createDescriptorSets(VulkanContext& context, VkImageView& textureImageView, VkSampler& textureSampler);
};

