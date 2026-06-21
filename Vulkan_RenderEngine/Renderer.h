#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include "FrameData.h"
#include "Swapchain.h"
#include "GraphicsState.h"
#include "RenderObject.h"
#include "Camera.h"

class ImGuiRenderer;

class VulkanRenderer
{
public:

	
	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int init(GLFWwindow* newWindow);
	void cleanup();

	void drawFrame(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
	void updateUniformBuffer(uint32_t currentImage, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

	bool framebufferResized = false;

	ImGuiRenderer* guiRenderer;

	VulkanContext context;
	Swapchain swapChain;
	RenderPass renderPass;
	Camera camera;
	FrameData frameData;

private:



	GraphicsPipeline graphicsPipeline;

	CommandPool commandPool;

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

