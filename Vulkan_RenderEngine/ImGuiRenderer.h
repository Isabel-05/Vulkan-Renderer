#pragma once
#include "Renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

class ImGuiRenderer
{
public:
	ImGuiRenderer() = default;
	~ImGuiRenderer() = default;

	void init(VulkanRenderer& renderer, ImGui_ImplVulkanH_Window* wd);
	void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VulkanRenderer& renderer, int width, int height);
	void cleanup();

	void FrameRender(ImGui_ImplVulkanH_Window* wd, VulkanRenderer& renderer, ImDrawData* draw_data);
	void FramePresent(ImGui_ImplVulkanH_Window* wd, VulkanRenderer& renderer);

private:
	void createDescriptorPool(VulkanContext& context);
	void check_vk_result(VkResult err);

	int frameBufferWidth = 0;
	int frameBufferHeight = 0;


	std::vector<VkCommandBuffer> commandBuffers;
	VkDescriptorPool descriptorPool;
};

