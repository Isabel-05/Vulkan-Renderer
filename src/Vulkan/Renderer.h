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
#include "IdRenderpass.h"


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

	void switchEditorState();


private:
	EditorState editorState = EditorState::Object;

	std::unique_ptr<ImGuiRenderer> guiRenderer;
	VkExtent2D viewportExtent = { 1, 1 };

	Camera camera;
	Swapchain swapChain;
	VulkanContext context;
	IdRenderpass idPass;
	ShaderResources shaderResources;
	CommandPool commandPool;
	FrameData frameData;

	Scene scene;
	std::vector<RenderObject> renderObjects;

	//Recording/Rendering functions
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void recordIdPass(VkCommandBuffer& cmdBuffer, uint32_t currentFrame, VkDescriptorSet& cameraDS);
	void recordOutlinePass(VkCommandBuffer& cmdBuffer, uint32_t currentFrame);

	//update functions
	void updateObjects();
	void resizeViewportResources();
	void updateUniformBuffer(uint32_t currentImage, glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

	//selection
	void updateSelection(uint32_t id);
	uint32_t pickId(uint32_t currentFrame, uint32_t pixelX, uint32_t pixelY);

	//helper functions
	bool checkViewportResize();
	void toColorAt(VkCommandBuffer& cmdBuffer, VkImage& image);
	void toShaderRead(VkCommandBuffer& cmdBuffer, VkImage& image);


	//tracking variables
	PickState pick;
	uint32_t currentFrame = 0;
	bool framebufferResized = false;
	glm::mat4 invViewProj;

	float inputScale = 1.0f;

	//Temp Bullshit
	float ct = 0;
	bool moveFlag = false;
};

