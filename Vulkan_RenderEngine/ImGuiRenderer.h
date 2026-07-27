#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/include/glfw3.h>
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

class VulkanContext;


class ImGuiRenderer
{
public:
	ImGuiRenderer(VulkanContext& vContext, uint32_t maxFramesInFlight);
	~ImGuiRenderer();

	void init(float width, float height);
	void cleanup();                                            
	void initResources();                                                          
	void initTexture();
	void setStyle();
	void updateTexture(CommandPool& cmdPool, ImTextureData* tex);                 // Dynamically update/create textures 
	void loadOutputImages(VkSampler& sampler, std::vector<VkImageView>& outputImageViews);

	// Frame-by-frame rendering operations
	void newFrame(uint32_t currentFrame);                                         // Begin new ImGui frame and generate geometry
	void updateBuffers(uint32_t currentFrame, uint32_t maxFramesInFlight);   // Upload updated geometry to GPU buffers
	void recordCmdBuffer(uint32_t currentFrame, VkCommandBuffer& commandBuffer, CommandPool& cmdPool, VkImageView& imageView); // Record rendering commands to command buffer

	// Input event handling for interactive UI elements
	void handleKey(int key, int scancode, int action, int mods);
	void handleMousePos(float x, float y);                  
	void handleMouseButton(int button, bool pressed);       
	bool getWantKeyCapture();                               // Query if ImGui wants keyboard focus
	void charPressed(uint32_t key);                         // Handle character input for text widgets

private:

	std::vector<VkDescriptorSet> viewportDescriptorSets;
	std::vector<ImTextureID> viewportTextureIds;

	// Core GPU rendering resources for UI display
	std::vector<VkBuffer> vertexBuffers;                                    
	std::vector<VkBuffer> indexBuffers;   
	std::vector<VkDeviceMemory> vertexBufferMemories;
	std::vector<VkDeviceMemory> indexBufferMemories;
	std::vector<uint32_t> vertexCounts;                              
	std::vector<uint32_t> indexCounts;

	VkSampler sampler = VK_NULL_HANDLE;						// Texture sampling configuration for font rendering
	VkImage fontImage;                                      // GPU texture containing ImGui font atlas
	VkDeviceMemory fontImageMemory;                             
	VkImageView fontImageView;               

	// Vulkan pipeline infrastructure for UI rendering
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;        // Pipeline compilation cache for faster startup
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;      // Resource binding layout (textures, uniforms)
	VkPipeline pipeline = VK_NULL_HANDLE;                  // Complete graphics pipeline for UI rendering
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;      // Pool for allocating descriptor sets
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // Layout defining shader resource bindings
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;        // Actual resource bindings for font texture

	VulkanContext* context;
	uint32_t graphicsQueueFamily = 0;                      // Queue family index for validation

	ImGuiStyle vulkanStyle;                                 // Custom visual styling for Vulkan applications

	// Push constants for efficient per-frame parameter updates
	// This structure enables fast updates of transformation and styling data
	struct PushConstBlock {
		glm::vec2 scale;                                    // UI scaling factors for different screen sizes
		glm::vec2 translate;                                // Translation offset for UI positioning
	} pushConstBlock;

	// Modern Vulkan rendering configuration
	VkPipelineRenderingCreateInfo renderingInfo{};        // Dynamic rendering setup parameters
	VkFormat colorFormat = VkFormat::VK_FORMAT_B8G8R8A8_SRGB;   // Target framebuffer format

};

