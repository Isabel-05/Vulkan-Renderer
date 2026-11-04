#pragma once
#include "GpuMeshCache.h"
#include "FrameData.h"

struct GpuMaterial
{
	std::shared_ptr<VkPipeline> pipeline;
	std::shared_ptr<VkPipelineLayout> pipelineLayout;
};

struct RenderObject
{
	GpuMeshCache gpuCache;

	GpuMaterial surfaceMaterial;
	GpuMaterial pointMaterial;
	GpuMaterial edgeMaterial;

	void init(VulkanContext& context, CommandPool& cmdPool, std::shared_ptr<DMesh> dmesh);
	void cleanup(VulkanContext& context);

	glm::mat4 getModelMatrix() const;
	void sync(VulkanContext& context, CommandPool& cmdPool, bool wantOverlay);
	void drawSurface(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS);
	void drawEdges(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS);
	void drawPoints(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS);
};