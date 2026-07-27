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

	void createSwapchain(VulkanContext& context);
	void createImageViews(VulkanContext& context);
	void createDepthResources(VulkanContext& context, CommandPool& cmdPool);
	void cleanupSwapChain(VulkanContext& context);
	void recreateSwapChain(VulkanContext& context, CommandPool& cmdPool);

private:

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(VulkanContext& context, const VkSurfaceCapabilitiesKHR& capabilities);
};

