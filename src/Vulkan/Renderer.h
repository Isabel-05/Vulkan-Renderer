#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include "FrameData.h"
#include "Swapchain.h"
#include "RenderObject.h"
#include "Camera.h"
#include "Scene.h" 
#include "ImGuiRenderer.h"
#include "ShaderResources.h"
#include "DMesh.h"
#include "SelectionState.h"
#include "IdRenderpass.h"

#include <memory>

enum class EditorState { Object, Edit};

struct PickState { bool wasClicked; uint32_t x; uint32_t y; };

struct IDPushConstants { glm::mat4 model; uint32_t id; };

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
	EditorState editorState = EditorState::Object;

	std::unique_ptr<ImGuiRenderer> guiRenderer;

	Camera camera;
	Swapchain swapChain;
	VulkanContext context;
	ShaderResources shaderResources;
	CommandPool commandPool;
	FrameData frameData;

	Scene scene;
	std::vector<RenderObject> renderObjects;

	Selection selection;
	IdRenderpass idPass;

	uint32_t currentFrame = 0;
	bool framebufferResized = false;

	float inputScale = 1.0f;

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void updateUniformBuffer(uint32_t currentImage, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

	void updateObjects();
	uint32_t pickId(VkDescriptorSet& cameraDS, uint32_t pixelX, uint32_t pixelY);
	PickState pick;
};

