#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include "FrameData.h"
#include "Swapchain.h"
#include "GraphicsState.h"
#include "RenderObject.h"
#include "Camera.h"
#include "Scene.h"

class ImGuiRenderer;

class VulkanRenderer
{
public:

	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int init(GLFWwindow* newWindow);
	void cleanup();

	void drawFrame(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

	bool framebufferResized = false;


	ImGuiRenderer* guiRenderer;
	VulkanContext context;
	Swapchain swapChain;
	Camera camera;

private:

	GraphicsPipeline graphicsPipeline;
	CommandPool commandPool;
	FrameData frameData;

	Scene scene;

	uint32_t currentFrame = 0;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void updateUniformBuffer(uint32_t currentImage, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
};

