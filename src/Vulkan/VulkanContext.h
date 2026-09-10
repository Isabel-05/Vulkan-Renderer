#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <stdexcept>

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

class VulkanContext
{

public:

	VulkanContext() = default;
	~VulkanContext() = default;

	GLFWwindow* window;

	VkInstance instance;


	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSurfaceKHR surface;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		#ifdef __APPLE__
   			"VK_KHR_portability_subset"
		#endif
	};

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};


	void init(GLFWwindow* newWindow);
	void cleanup();

	void createInstance();
	void getPhysicalDevice();
	void createLogicalDevice();
	void createSurface();

	QueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:

	bool checkInstanceExtensionSupport(std::vector<const char*>* extensionsToCheck);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkDeviceSuitable(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUsableMsaaSampleCount();
	bool checkValidationLayerSupport();


};

