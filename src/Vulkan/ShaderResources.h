#pragma once
#include "VulkanContext.h"

class ShaderResources
{

	void cleanup(VulkanContext &context);

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout cameraDSLayout;
	std::vector<VkDescriptorSet> cameraDescriptorSets;


	void createDescriptorSetLayouts(VulkanContext& context);
	void createDescriptorPool(VulkanContext& context);
	void createDescriptorSets(VulkanContext& context);
};