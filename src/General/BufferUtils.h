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

	template <typename T>
	void uploadBufferToGpu(VulkanContext& context, CommandPool& cmdPool, VkBufferUsageFlags usage,
		const std::vector<T> inputData, VkBuffer& outBuffer, VkDeviceMemory& outMemory)
	{
		VkDeviceSize bufferSize = sizeof(inputData[0]) * inputData.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		BufferUtils::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(context.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, inputData.data(), (size_t)bufferSize);
		vkUnmapMemory(context.logicalDevice, stagingBufferMemory);

		BufferUtils::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outBuffer, outMemory);

		BufferUtils::copyBuffer(context, cmdPool, stagingBuffer, outBuffer, bufferSize);

		vkDestroyBuffer(context.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(context.logicalDevice, stagingBufferMemory, nullptr);
	}
}
