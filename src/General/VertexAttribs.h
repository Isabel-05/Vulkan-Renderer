#pragma once
#include "VulkanContext.h"
#include <array>


namespace VertexAttribs
{
	VkVertexInputBindingDescription getBindingDescription();
	std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();
}