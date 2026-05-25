#pragma once
#include "BufferUtils.h"

namespace ImageUtils
{
	void createImage(VulkanContext& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VulkanContext& context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkImage createTextureImage(VulkanContext& context, CommandPool& cmdPool, std::string texPath);
	VkSampler createImageSampler(VulkanContext& context);
	void transitionImageLayout(VulkanContext& context, CommandPool& cmdPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
}
