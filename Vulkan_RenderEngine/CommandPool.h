#pragma once
#include "VulkanContext.h"

class CommandPool
{
public:

	CommandPool() = default;
	~CommandPool() = default;

    void create(VulkanContext& context);
    void destroy(VulkanContext& context);

    VkCommandBuffer beginSingleTimeCommands(VulkanContext& context);
    void endSingleTimeCommands(VulkanContext& context, VkCommandBuffer commandBuffer);

    VkCommandPool handle;
    std::vector<VkCommandBuffer> buffers;
};

