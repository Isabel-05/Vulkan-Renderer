#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Utilities.h"

class VulkanRenderer
{
public:
	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void cleanup();

	

private:

	GLFWwindow* window;

	//Vulkan Components
	VkInstance instance;
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} 
	mainDevice;

	VkQueue graphicsQueue;

	//Vulkan Functions

	//createFunctions
	void createInstance();
	void createLogicalDevice();

	//get Functions
	void getPhysicalDevice();

	//support Functions
	bool checkInstanceExtensionSupport(std::vector<const char*> *extensionsToCheck);
	bool checkDeviceSuitable(VkPhysicalDevice device);

	QueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice device);
};

