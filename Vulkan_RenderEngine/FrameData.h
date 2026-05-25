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
	FrameData();
	~FrameData();

	void cleanup(VulkanContext& context);

	uint32_t maxFramesInFlight;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkDeviceMemory> commandBuffersMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;


	void createUniformBuffers(VulkanContext& context);
	void createCommandBuffers(VulkanContext& context, CommandPool& cmdPool);
	void createSyncObjects(VulkanContext& context, size_t imageCount);
};

