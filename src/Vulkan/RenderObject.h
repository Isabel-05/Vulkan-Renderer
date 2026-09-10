#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

#include "DMesh.h"
#include "FrameData.h"
#include <string>
#include <memory>


struct GpuMesh
{
	std::shared_ptr<DMesh> dataMesh;

	std::vector<Vertex> evalVertices;
	std::vector<uint32_t> evalIndices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	uint32_t indexCount;

	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh);
	void upload(VulkanContext& context, CommandPool& cmdPool, std::vector<Vertex> verts, std::vector<uint32_t> indices);
	void cleanup(VulkanContext& context);
};

struct GpuMaterial
{
	void init(VulkanContext& context, CommandPool& cmdPool);

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
	std::vector<GpuMaterial> materials;

	glm::mat4 getModelMatrix() const;

	void checkAndUpdateMesh(VulkanContext& context, CommandPool& cmdPool);
	void draw(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, VkDescriptorSet& cameraDS);

	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh);

	void cleanup(VulkanContext& context);
};

