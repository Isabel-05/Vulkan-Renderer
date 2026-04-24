#pragma once

struct QueueFamilyIndices
{
	uint32_t graphicsFamily = -1;
	uint32_t presentFamily = -1;

	bool isValid()
	{
		return (graphicsFamily >= 0) && (presentFamily >= 0);
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};