#pragma once
#include "FrameData.h"
#include <array>



void FrameData::cleanup(VulkanContext& context, size_t imageCount)
{
	for (size_t i = 0; i < maxFramesInFlight; i++) {
		vkDestroyBuffer(context.logicalDevice, uniformBuffers[i], nullptr);
		vkFreeMemory(context.logicalDevice, uniformBuffersMemory[i], nullptr);
	}

	//destroying descriptor pool also implicitly frees the descriptor sets allocated from it, so we dont have to free those individually
	vkDestroyDescriptorPool(context.logicalDevice, descriptorPool, nullptr);


	vkDestroyDescriptorSetLayout(context.logicalDevice, cameraDSLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.logicalDevice, materialDSLayout, nullptr);

	for (size_t i = 0; i < imageCount; i++) {
		vkDestroySemaphore(context.logicalDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(context.logicalDevice, imageAvailableSemaphores[i], nullptr);
	}
	for (size_t i = 0; i < maxFramesInFlight; i++) {
		vkDestroyFence(context.logicalDevice, inFlightFences[i], nullptr);
	}
}

void FrameData::createUniformBuffers(VulkanContext& context)
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(maxFramesInFlight);
	uniformBuffersMemory.resize(maxFramesInFlight);
	uniformBuffersMapped.resize(maxFramesInFlight);

	for (size_t i = 0; i < maxFramesInFlight; i++) {
		BufferUtils::createBuffer(context, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

		vkMapMemory(context.logicalDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
	}
}

void FrameData::createCommandBuffers(VulkanContext& context, CommandPool& cmdPool)
{
	commandBuffers.resize(maxFramesInFlight);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = cmdPool.handle;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(context.logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void FrameData::createSyncObjects(VulkanContext& context, size_t imageCount)
{
	imageAvailableSemaphores.resize(imageCount);
	renderFinishedSemaphores.resize(imageCount);
	inFlightFences.resize(maxFramesInFlight);


	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // makes sure the first draw call doesnt infinitely wait

	for (size_t i = 0; i < imageCount; i++) {
		if (vkCreateSemaphore(context.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(context.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
	for (size_t i = 0; i < maxFramesInFlight; i++) {
		if (vkCreateFence(context.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void FrameData::createDescriptorSetLayouts(VulkanContext& context)
{
	//uniform buffer layout for Model view projection matrices
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding cameraBinding = uboLayoutBinding;
	VkDescriptorSetLayoutCreateInfo cameralayoutInfo{};
	cameralayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	cameralayoutInfo.bindingCount = 1;
	cameralayoutInfo.pBindings = &cameraBinding;

	if (vkCreateDescriptorSetLayout(context.logicalDevice, &cameralayoutInfo, nullptr, &cameraDSLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	//texture sampler descriptor set layout binding
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding materialBinding = samplerLayoutBinding;
	VkDescriptorSetLayoutCreateInfo materiallayoutInfo{};
	materiallayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	materiallayoutInfo.bindingCount = 1;
	materiallayoutInfo.pBindings = &materialBinding;

	if (vkCreateDescriptorSetLayout(context.logicalDevice, &materiallayoutInfo, nullptr, &materialDSLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void FrameData::createDescriptorPool(VulkanContext& context)
{
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(maxFramesInFlight) * MAX_NUMBER_OF_OBJECTS;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(maxFramesInFlight) * MAX_NUMBER_OF_OBJECTS;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(maxFramesInFlight + maxFramesInFlight * MAX_NUMBER_OF_OBJECTS);

	if (vkCreateDescriptorPool(context.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

}

void FrameData::createDescriptorSets(VulkanContext& context)
{
	std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, cameraDSLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(maxFramesInFlight);
	allocInfo.pSetLayouts = layouts.data();

	cameraDescriptorSets.resize(maxFramesInFlight);
	if (vkAllocateDescriptorSets(context.logicalDevice, &allocInfo, cameraDescriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < maxFramesInFlight; i++) {

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrites{};

		descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites.dstSet = cameraDescriptorSets[i];
		descriptorWrites.dstBinding = 0;
		descriptorWrites.dstArrayElement = 0;
		descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites.descriptorCount = 1;
		descriptorWrites.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context.logicalDevice, 1, &descriptorWrites, 0, nullptr);
	}
}