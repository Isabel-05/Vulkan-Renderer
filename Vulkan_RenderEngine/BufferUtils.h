#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

namespace BufferUtils
{
	void copyBuffer(VulkanContext& context, CommandPool& cmdPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VulkanContext& context, CommandPool& cmdPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void copyBufferToImage(VulkanContext& context, VkCommandBuffer& cmdBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void createBuffer(VulkanContext& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(VulkanContext& context, const std::vector<char>& code);
}
