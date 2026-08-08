#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <stdexcept>

#include "CommandPool.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "RenderObject.h"

class VulkanContext;


class ImGuiRenderer
{
public:
	ImGuiRenderer(VulkanContext& vContext, uint32_t maxFramesInFlight);
	~ImGuiRenderer() = default;

	void init(float width, float height);
	void cleanup();

	void initResources();

	//For Viewport Image
	void loadOutputImages(VkSampler& sampler, std::vector<VkImageView>& outputImageViews);
	void reloadOutputImages(VkSampler& sampler, std::vector<VkImageView>& outputImageViews);

	// Frame-by-frame rendering operations
	void newFrame(uint32_t currentFrame, std::vector<RenderObject>& objHierarchy, uint32_t& selectedObjId);
	void updateBuffers(uint32_t currentFrame, uint32_t maxFramesInFlight);
	void recordCmdBuffer(uint32_t currentFrame, VkCommandBuffer& commandBuffer, CommandPool& cmdPool, VkImageView& imageView); 

	// Input event handling
	void handleKey(int key, int scancode, int action, int mods);
	void handleMousePos(float x, float y);
	void handleMouseButton(int button, bool pressed);
	bool getWantKeyCapture();
	void charPressed(uint32_t key);

	bool isViewportHovered() { return viewportHovered; }

private:

	//init stuff
	void createPipeline();
	void initImguiVulkanImpl();
	void initTexture();
	void setStyle();

	//frame-by-frame update stuff
	void setupDockspace(ImGuiID dockspace_id);
	void updateTexture(CommandPool& cmdPool, ImTextureData* tex);

	std::vector<VkDescriptorSet> viewportDescriptorSets;
	std::vector<ImTextureID> viewportTextureIds;

	std::vector<VkBuffer> vertexBuffers;                                    
	std::vector<VkBuffer> indexBuffers;   
	std::vector<VkDeviceMemory> vertexBufferMemories;
	std::vector<VkDeviceMemory> indexBufferMemories;
	std::vector<uint32_t> vertexCounts;                              
	std::vector<uint32_t> indexCounts;

	VkSampler sampler = VK_NULL_HANDLE;
	VkImage fontImage;                                     
	VkDeviceMemory fontImageMemory;                             
	VkImageView fontImageView;               

	VkPipelineCache pipelineCache = VK_NULL_HANDLE;        // Pipeline compilation cache for faster startup
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;  
	VkPipeline pipeline = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;     
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	VulkanContext* context;
	uint32_t graphicsQueueFamily = 0;

	// Push constants for efficient per-frame parameter updates
	struct PushConstBlock {
		glm::vec2 scale;                                    // UI scaling factors for different screen sizes
		glm::vec2 translate;                                // Translation offset for UI positioning
	} pushConstBlock;


	VkPipelineRenderingCreateInfo renderingInfo{};    
	VkFormat colorFormat = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;

	bool viewportHovered = false;
};

