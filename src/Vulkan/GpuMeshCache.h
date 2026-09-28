#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"
#include "DMesh.h"
#include "BufferUtils.h"

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
		//TODO
		//Replace with mapped buffers
		vkDeviceWaitIdle(ctx.logicalDevice);

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

struct GpuSurfaceVertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

namespace SurfaceVertexAttribs
{
	VkVertexInputBindingDescription getBindingDescription();
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
}

namespace PointVertexAttribs
{
	//shared by the point-cloud and edge-list pipelines: position-only input
	VkVertexInputBindingDescription getBindingDescription();
	std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
}

struct GpuMeshCache
{
	std::shared_ptr<DMesh> dataMesh;

	//full mesh (triangulated, modifiers, etc)
	GpuBuffer surfaceVertBuffer;
	GpuBuffer surfaceIdxBuffer;

	//only datamesh points/edges (for edit moder overlay)
	GpuBuffer pointBuffer;
	GpuBuffer edgeIdxBuffer;

	void sync(VulkanContext& context, CommandPool& cmdPool, bool wantOverlay);

	void init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh);
	void cleanup(VulkanContext& context);

private:
	MeshVersion synced{ 0, 0, 0, 0, 0 }; //zeros so it doesnt match dmesh version on init
};