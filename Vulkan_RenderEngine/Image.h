#pragma once
#include "BufferUtils.h"

class RenderObject;

namespace ImageUtils
{
	void createImage(VulkanContext& context, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLvls = 1);
	void createImageView(VulkanContext& context, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView& imageView, uint32_t mipLvls);
	void createTextureImage(VulkanContext& context, CommandPool& cmdPool, std::string texPath, RenderObject& rdrObject);
	void createImageSampler(VulkanContext& context, VkSampler& textureSampler);
	void generateMipMaps(VulkanContext& context, CommandPool& cmdPool, VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void transitionImageLayout(VulkanContext& context, CommandPool& cmdPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLvls);
	void transitionImageLayout(VulkanContext& context, VkCommandBuffer& cmdBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLvls);
}
