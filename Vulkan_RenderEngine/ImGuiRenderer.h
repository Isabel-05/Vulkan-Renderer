#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/include/glfw3.h>
#include <vector>
#include <stdexcept>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

class VulkanRenderer;
class VulkanContext;

class ImGuiRenderer
{
public:
	ImGuiRenderer(VulkanRenderer& renderer);
	~ImGuiRenderer();

	// Core functionality methods for ImGui integration
	void init(float width, float height);                   // Initialize ImGui context and configure display
	void cleanup();                                            // Clean up ImGui resources and context
	void initResources();                                    // Create all Vulkan resources for rendering
	void setStyle(uint32_t index);                          // Apply visual styling themes
	void initTexture();
	void updateTexture(VkCommandBuffer& commandBuffer, ImTextureData* tex);                 // Dynamically update/create textures (v1.92+)

	// Frame-by-frame rendering operations
	bool newFrame();                                         // Begin new ImGui frame and generate geometry
	void updateBuffers(uint32_t currentFrame, uint32_t maxFramesInFlight);   // Upload updated geometry to GPU buffers
	void drawFrame(VkCommandBuffer& commandBuffer, VkImageView& imageView); // Record rendering commands to command buffer

	// Input event handling for interactive UI elements
	void handleKey(int key, int scancode, int action, int mods); // Process keyboard input events
	void handleMousePos(float x, float y);                  // Process mouse movement events (v1.87+)
	void handleMouseButton(int button, bool pressed);       // Process mouse button events (v1.87+)
	bool getWantKeyCapture();                               // Query if ImGui wants keyboard focus
	void charPressed(uint32_t key);                         // Handle character input for text widgets

private:

	// Core GPU rendering resources for UI display
	// These objects form the foundation of our ImGui-to-Vulkan rendering pipeline
	VkSampler sampler = VK_NULL_HANDLE;                    // Texture sampling configuration for font rendering
	std::vector<VkBuffer> vertexBuffers;                                    
	std::vector<VkBuffer> indexBuffers;   
	std::vector<VkDeviceMemory> vertexBufferMemories;
	std::vector<VkDeviceMemory> indexBufferMemories;
	uint32_t vertexCount = 0;                              // Current vertex count for draw commands
	uint32_t indexCount = 0;                               // Current index count for draw commands
	VkImage fontImage;                                        // GPU texture containing ImGui font atlas
	VkDeviceMemory fontImageMemory;                                // Memory backing the font texture
	VkImageView fontImageView;                                // Shader-accessible view of font texture

	// Vulkan pipeline infrastructure for UI rendering
	// These objects define the complete GPU processing pipeline for ImGui elements
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;        // Pipeline compilation cache for faster startup
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;      // Resource binding layout (textures, uniforms)
	VkPipeline pipeline = VK_NULL_HANDLE;                  // Complete graphics pipeline for UI rendering
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;      // Pool for allocating descriptor sets
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // Layout defining shader resource bindings
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;        // Actual resource bindings for font texture

	VulkanContext* context;
	uint32_t graphicsQueueFamily = 0;                      // Queue family index for validation

	// UI state management and rendering configuration
	// These members control the visual appearance and dynamic behavior of the UI system
	ImGuiStyle vulkanStyle;                                 // Custom visual styling for Vulkan applications

	// Push constants for efficient per-frame parameter updates
	// This structure enables fast updates of transformation and styling data
	struct PushConstBlock {
		glm::vec2 scale;                                    // UI scaling factors for different screen sizes
		glm::vec2 translate;                                // Translation offset for UI positioning
	} pushConstBlock;

	// Dynamic state tracking for performance optimization
	bool needsUpdateBuffers = false;                        // Flag indicating buffer resize requirements

	// Modern Vulkan rendering configuration
	VkPipelineRenderingCreateInfo renderingInfo{};        // Dynamic rendering setup parameters
	VkFormat colorFormat = VkFormat::VK_FORMAT_B8G8R8A8_UNORM;   // Target framebuffer format

};

