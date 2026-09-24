#pragma once
#include "VulkanContext.h"
#include "Image.h"

class Swapchain
{
public:
	Swapchain() = default;
	~Swapchain() = default;

	void cleanup(VulkanContext& context);

	VkSwapchainKHR handle;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkImage depthResolveImage;
	VkDeviceMemory depthResolveMemory;
	VkImageView depthResolveView;

	VkFormat imageFormat;
	VkExtent2D extent;
	uint32_t imageCount;

	//for msaa intermediate texture
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	//Offscreen texture to be sampled by imgui
	std::vector<VkImage> outputImages;
	std::vector<VkImageView> outputImageViews;
	std::vector<VkDeviceMemory> outputImageMemories;

	VkSampler outputSampler;

	//Swapchain
	void createSwapchain(VulkanContext& context);
	void createImageViews(VulkanContext& context);

	//Render Attachments
	void createDepthResources(VulkanContext& context, CommandPool& cmdPool, VkExtent2D renderExtent);
	void createColorResources(VulkanContext& context, CommandPool& cmdPool, VkExtent2D renderExtent);
	void createOutputResources(VulkanContext& context, uint32_t maxFramesInFlight, VkExtent2D renderExtent);

	void cleanupSwapChain(VulkanContext& context);
	void cleanupViewportImages(VulkanContext& context);
	void recreateSwapChain(VulkanContext& context, CommandPool& cmdPool, uint32_t maxFramesInFlight);

private:

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(VulkanContext& context, const VkSurfaceCapabilitiesKHR& capabilities);
};

