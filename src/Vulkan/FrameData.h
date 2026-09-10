#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include <glm/glm.hpp>


struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
};


class FrameData
{
public:
	FrameData() = default;
	~FrameData() = default;

	void cleanup(VulkanContext& context, size_t imageCount);

	const uint32_t maxFramesInFlight = 2;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkDeviceMemory> commandBuffersMemory;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	std::vector<VkDescriptorSet> cameraDescriptorSets;

	void createUniformBuffers(VulkanContext& context);

	void createCommandBuffers(VulkanContext& context, CommandPool& cmdPool);
	void createSyncObjects(VulkanContext& context, size_t imageCount);

	void createDescriptorSets(VulkanContext& context, VkDescriptorPool& DsPool, VkDescriptorSetLayout& DsLayout);
};

