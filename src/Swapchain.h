#pragma once
#include "VulkanContext.h"
#include "Image.h"

class Swapchain
{
public:
	Swapchain() = default;
	~Swapchain() = default;

	VkSwapchainKHR handle;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	VkFormat imageFormat;
	VkExtent2D extent;
	uint32_t imageCount;

	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;

	//Output
	std::vector<VkImage> outputImages;
	std::vector<VkImageView> outputImageViews;
	std::vector<VkDeviceMemory> outputImageMemories;

	VkSampler outputSampler;


	void createSwapchain(VulkanContext& context);
	void createImageViews(VulkanContext& context);
	void createDepthResources(VulkanContext& context, CommandPool& cmdPool);
	void createColorResources(VulkanContext& context, CommandPool& cmdPool);
	void createOutputResources(VulkanContext& context, uint32_t maxFramesInFlight);
	void cleanupSwapChain(VulkanContext& context);
	void recreateSwapChain(VulkanContext& context, CommandPool& cmdPool, uint32_t maxFramesInFlight);

private:

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(VulkanContext& context, const VkSurfaceCapabilitiesKHR& capabilities);
};

