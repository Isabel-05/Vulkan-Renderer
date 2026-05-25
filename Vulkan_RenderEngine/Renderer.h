#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include "FrameData.h"
#include "Swapchain.h"
#include "GraphicsState.h"
#include "RenderObject.h"

class VulkanRenderer
{
public:

	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int init(GLFWwindow* newWindow);
	void cleanup();

	void drawFrame();
	void updateUniformBuffer(uint32_t currentImage);

	bool framebufferResized = false;
	VulkanContext context;

private:


	Swapchain swapChain;
	GraphicsPipeline graphicsPipeline;
	RenderPass renderPass;
	CommandPool commandPool;
	FrameData frameData;

	RenderObject testObject;

	uint32_t currentFrame = 0;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void createVertexBuffer();
	void createIndexBuffer();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};

