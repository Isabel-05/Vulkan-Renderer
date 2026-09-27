#pragma once
#include "GpuMeshCache.h"
#include "FrameData.h"

struct GpuMaterial
{
	std::shared_ptr<VkPipeline> pipeline;
	std::shared_ptr<VkPipelineLayout> pipelineLayout;
};

class RenderObject
{
public:
	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh);
	void cleanup(VulkanContext& context);

	std::string name;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	GpuMeshCache gpuCache;

	GpuMaterial surfaceMaterial;
	GpuMaterial pointMaterial;
	GpuMaterial edgeMaterial;

	glm::mat4 getModelMatrix() const;

	void sync(VulkanContext& context, CommandPool& cmdPool, bool wantOverlay);
	void drawSurface(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS);
	void drawEdges(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS);
	void drawPoints(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS);
};