#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include "FrameData.h"
#include "Swapchain.h"
#include "GraphicsState.h"
#include "RenderObject.h"
#include "Camera.h"
#include "Scene.h"
#include "ImGuiRenderer.h"

#include <memory>

class VulkanRenderer
{
public:

	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int init(GLFWwindow* newWindow);
	void cleanup();

	void drawFrame();

	//Callbacks
	void onResize();
	void onKey(int key, int scancode, int action, int mods);
	void onMouseMove(double xpos, double ypos, float xoffset, float yoffset);
	void onMousePressed(int button, int action, int mods);
	void onMouseWheel(double xoffset, double yoffset);


private:

	std::unique_ptr<ImGuiRenderer> guiRenderer;

	Camera camera;
	Swapchain swapChain;
	VulkanContext context;
	GraphicsPipeline graphicsPipeline;
	CommandPool commandPool;
	FrameData frameData;

	Scene scene;

	uint32_t currentFrame = 0;
	bool framebufferResized = false;

	float inputScale = 1.0f;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void updateUniformBuffer(uint32_t currentImage, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
};

