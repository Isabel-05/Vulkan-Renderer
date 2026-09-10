#include "ShaderResources.h"


void ShaderResources::cleanup(VulkanContext& context)
{
	//destroying descriptor pool also implicitly frees the descriptor sets allocated from it, so we dont have to free those individually
	vkDestroyDescriptorPool(context.logicalDevice, descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(context.logicalDevice, cameraDSLayout, nullptr);
}

void ShaderResources::createDescriptorSetLayouts(VulkanContext& context)
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
}

void ShaderResources::createDescriptorPool(VulkanContext& context)
{
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1000;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1000;

	if (vkCreateDescriptorPool(context.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void ShaderResources::createDescriptorSets(VulkanContext& context)
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