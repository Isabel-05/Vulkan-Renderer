#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

#include "DMesh.h"
#include "Modifiers.h"
#include "FrameData.h"
#include <string>
#include <memory>

struct GpuBuffer {

	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize capacity = 0;   // bytes currently allocated
	uint32_t count = 0;          // element count currently valid

	void cleanup(VulkanContext& context);

	template <typename T>
	void uploadOrResize(VulkanContext& ctx, CommandPool& pool, const std::vector<T>& data, VkBufferUsageFlags usage)
	{
		VkDeviceSize needed = sizeof(T) * data.size();
		if (needed == 0) return;

		if (needed > capacity)
		{
			if (buffer) { vkDestroyBuffer(ctx.logicalDevice, buffer, nullptr); vkFreeMemory(ctx.logicalDevice, memory, nullptr); }
			BufferUtils::createBuffer(ctx, needed, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory);
			capacity = needed;
		}

		VkBuffer staging; VkDeviceMemory stagingMem;
		BufferUtils::createBuffer(ctx, needed, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging, stagingMem);
		void* mapped; vkMapMemory(ctx.logicalDevice, stagingMem, 0, needed, 0, &mapped);
		memcpy(mapped, data.data(), (size_t)needed);
		vkUnmapMemory(ctx.logicalDevice, stagingMem);

		BufferUtils::copyBuffer(ctx, pool, staging, buffer, needed);

		vkDestroyBuffer(ctx.logicalDevice, staging, nullptr);
		vkFreeMemory(ctx.logicalDevice, stagingMem, nullptr);
		count = (uint32_t)data.size();
	}
};

struct GpuMesh
{
	std::shared_ptr<DMesh> dataMesh;

	std::vector<Vertex> evalVertices;
	std::vector<uint32_t> evalIndices;

	GpuBuffer surfaceVertBuffer;
	GpuBuffer surfaceIdxBuffer;
	uint32_t surfaceIdxCount;

	GpuBuffer pointBuffer;
	GpuBuffer edgeIdxs;
	

	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh);
	void upload(VulkanContext& context, CommandPool& cmdPool, std::vector<Vertex> verts, std::vector<uint32_t> indices);
	void cleanup(VulkanContext& context);
};

struct GpuMaterial
{
	GpuMaterial() = default;
	~GpuMaterial() = default;

	void init();

	std::shared_ptr<VkPipeline> pipeline;
	std::shared_ptr<VkPipelineLayout> pipelineLayout;
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
	void draw(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS);

	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh);

	void cleanup(VulkanContext& context);
};