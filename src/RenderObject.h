#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

#include "DMesh.h"
#include "DMaterial.h"
#include "FrameData.h"
#include <string>
#include <memory>


struct GpuMesh
{
	std::unique_ptr<DMesh> dataMesh;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	uint32_t indexCount;

	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh);
	void upload(VulkanContext& context, CommandPool& cmdPool);
	void cleanup(VulkanContext& context);
};

struct GpuMaterial
{
	std::unique_ptr<DMaterial> dataMaterial;

	VkImage texture;
	VkDeviceMemory textureMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	uint32_t mipLevels;

	VkDescriptorSet descriptorSet;

	void createDescriptorSets(VulkanContext& context, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout);
	void updateDescriptorSets(VulkanContext& context, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout);

	void initTexResources(VulkanContext& context, CommandPool& cmdPool, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout);
	void init(VulkanContext& context, CommandPool& cmdPool, DMaterial& dmaterial, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout);

	void cleanupTexResources(VulkanContext& context);
	void cleanup(VulkanContext& context);
};

class RenderObject
{
public:

	std::string name;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	GpuMesh mesh;
	GpuMaterial material;

	glm::mat4 getModelMatrix() const;

	void draw(VulkanContext& context, CommandPool& cmdPool, FrameData& frameData, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, VkDescriptorSet& cameraDS);

	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh, DMaterial& dmaterial,
		VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout);

	void cleanup(VulkanContext& context);
};

