#pragma once
#include "Swapchain.h"
#include "imgui.h"
#include <glm/glm.hpp>

void Swapchain::createSwapchain(VulkanContext& context)
{
	SwapChainSupportDetails swapChainSupport = context.querySwapChainSupport(context.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D swapChainExtent = chooseSwapExtent(context, swapChainSupport.capabilities);

	uint32_t swapChainimageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && swapChainimageCount > swapChainSupport.capabilities.maxImageCount) {
		swapChainimageCount = swapChainSupport.capabilities.maxImageCount;
	}

	imageCount = swapChainimageCount;

	//Info struct
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = context.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapChainExtent;
	createInfo.imageArrayLayers = 1; //always one unless youre working on a 3d stereoscopic apllication
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //specifies what the image in the swap chain will be used for (rendering on screen/into file etc)

	QueueFamilyIndices indices = context.getQueueFamilyIndices(context.physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}
	//you can add a transform to the image before it gets rendered. if you dont want that just use the code below
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	//ignore alpha channel for blending with other windows
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	//clips pixels if they are obscured by another window for example
	createInfo.clipped = VK_TRUE;

	//TODO: If window gets resized (bzw. new swap chain gets initialized) keep handle to old swapchain
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	//create swapchain
	if (vkCreateSwapchainKHR(context.logicalDevice, &createInfo, nullptr, &handle) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	//save images in member
	vkGetSwapchainImagesKHR(context.logicalDevice, handle, &imageCount, nullptr);
	images.resize(imageCount);
	vkGetSwapchainImagesKHR(context.logicalDevice, handle, &imageCount, images.data());

	//save format and extent in member
	imageFormat = surfaceFormat.format;
	extent = swapChainExtent;
}

void Swapchain::createImageViews(VulkanContext& context)
{
	imageViews.resize(images.size());

	for (size_t i = 0; i < images.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = imageFormat;
		//you can optonally play around with the channels of the image. in our case we use default
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		//this field describes what the image's purpose is and which part of the image should be accessed.
		//Our images will be used as color targets without any mipmapping levels or multiple layers.
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;


		//create imageview
		if (vkCreateImageView(context.logicalDevice, &createInfo, nullptr, &imageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void Swapchain::createDepthResources(VulkanContext& context, CommandPool& cmdPool)
{
	ImageUtils::createImage(context, extent.width, extent.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory, 1, context.msaaSamples);
	ImageUtils::createImageView(context, depthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView, 1);

	ImageUtils::transitionImageLayout(context, cmdPool, depthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
}

void Swapchain::createColorResources(VulkanContext& context, CommandPool& cmdPool)
{
	ImageUtils::createImage(context, extent.width, extent.height, imageFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory, 1, context.msaaSamples);
	ImageUtils::createImageView(context, colorImage, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, colorImageView, 1);

	ImageUtils::transitionImageLayout(context, cmdPool, colorImage, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

}

void Swapchain::cleanupSwapChain(VulkanContext& context)
{
	vkDestroyImage(context.logicalDevice, depthImage, nullptr);
	vkFreeMemory(context.logicalDevice, depthImageMemory, nullptr);
	vkDestroyImageView(context.logicalDevice, depthImageView, nullptr);

	vkDestroyImage(context.logicalDevice, colorImage, nullptr);
	vkDestroyImageView(context.logicalDevice, colorImageView, nullptr);
	vkFreeMemory(context.logicalDevice, colorImageMemory, nullptr);

	//Each image view needs to be deleted individually
	for (auto imageView : imageViews) {
		vkDestroyImageView(context.logicalDevice, imageView, nullptr);
	}

	//Images deleted automatically with the swapchain
	vkDestroySwapchainKHR(context.logicalDevice, handle, nullptr);

}

void Swapchain::recreateSwapChain(VulkanContext& context, CommandPool& cmdPool)
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(context.window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(context.window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(context.logicalDevice);
	cleanupSwapChain(context);

	createSwapchain(context);
	createImageViews(context);
	createColorResources(context, cmdPool);
	createDepthResources(context, cmdPool);
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(static_cast<float>(extent.width),
		static_cast<float>(extent.height));
}


VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR /*VK_PRESENT_MODE_MAILBOX_KHR*/) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(VulkanContext& context, const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(context.window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = glm::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = glm::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}