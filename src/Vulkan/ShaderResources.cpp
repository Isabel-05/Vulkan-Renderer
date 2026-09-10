#include "ShaderResources.h"
#include "MaterialUtils.h"


void ShaderResources::cleanup(VulkanContext& context)
{
	//destroying descriptor pool also implicitly frees the descriptor sets allocated from it, so we dont have to free those individually
	vkDestroyDescriptorPool(context.logicalDevice, descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(context.logicalDevice, cameraDSLayout, nullptr);

	vkDestroyPipeline(context.logicalDevice, BaseShaderPl, nullptr);
	vkDestroyPipelineLayout(context.logicalDevice, BaseShaderLayout, nullptr);

	//vkDestroyPipeline(context.logicalDevice, LineShaderPl, nullptr);
	//vkDestroyPipelineLayout(context.logicalDevice, LineShaderLayout, nullptr);

	//vkDestroyPipeline(context.logicalDevice, PointShaderPl, nullptr);
	//vkDestroyPipelineLayout(context.logicalDevice, PointShaderLayout, nullptr);

	//vkDestroyPipeline(context.logicalDevice, OutlineShaderPl, nullptr);
	//vkDestroyPipelineLayout(context.logicalDevice, OutlineShaderLayout, nullptr);
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

void ShaderResources::createPipelines(VulkanContext& context, const VkFormat& swapchainFormat)
{
	MaterialUtils::createPipeline(
		context,
		std::string(SHADER_DIR) + "vert.spv",
		std::string(SHADER_DIR) + "frag.spv",
		cameraDSLayout,
		VK_POLYGON_MODE_FILL,
		swapchainFormat,
		BaseShaderPl,
		BaseShaderLayout
	);
}

