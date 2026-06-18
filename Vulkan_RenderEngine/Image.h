#pragma once
#include "BufferUtils.h"

namespace ImageUtils
{
	void createImage(VulkanContext& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void createImageView(VulkanContext& context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView);
	void createTextureImage(VulkanContext& context, CommandPool& cmdPool, std::string texPath, VkImage& textureImage, VkDeviceMemory& textureImageMemory);
	void createImageSampler(VulkanContext& context, VkSampler& textureSampler);
	void transitionImageLayout(VulkanContext& context, CommandPool& cmdPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void transitionImageLayout(VulkanContext& context, VkCommandBuffer& cmdBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
}
