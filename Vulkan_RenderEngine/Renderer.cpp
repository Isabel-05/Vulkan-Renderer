#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/include/glfw3.h>

#include "Renderer.h"

VulkanRenderer::VulkanRenderer()
{
}

VulkanRenderer::~VulkanRenderer()
{
}


int VulkanRenderer::init(GLFWwindow* newWindow)
{
	window = newWindow;

	try {
		createInstance();
		getPhysicalDevice();
		createLogicalDevice();
	}
	catch (const std::runtime_error& e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}


	return 0;
}

void VulkanRenderer::cleanup()
{
	//destroy instance and device without custom deallocator
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);

}


void VulkanRenderer::createInstance()
{
	//Info Struct containing info about the application
	//most data here doesnt affect the program and is for developer convenience (look up stuff)
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_4;

	//Create Info struct to be passed into instance creation function
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	//setup the extensions the vulkan instance will use

	//create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;		//extensions passed as array of cstrings
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount); //GLFW function that returns the vulkan extensions it needs

	//Add GLFW extensions to the list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	//Check if instance extensions are supported
	if (!checkInstanceExtensionSupport(&instanceExtensions)) 
	{
		throw std::runtime_error("VkInstance does not support one or more instance extensions");
	}

	//add extensions to instance create info struct
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	//optional TODO: set up Validation Layers (for Debug)
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	//Actually Create Instance from created Info struct (stored in instance variable)
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

	if (result != VK_SUCCESS) 
	{
		throw std::runtime_error("Failed to create Vulkan Instanc");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	QueueFamilyIndices indices = getQueueFamilyIndices(mainDevice.physicalDevice);

	//Queues the logical device needs
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
	queueCreateInfo.queueCount = 1;
	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;

	//information to create logical device (often called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;		//number of queue create info structs
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = 0;		//number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;
	
	//physical device features the logical device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	//Create logical device
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Logical Device");
	}

	//from given logical device grab Queue and store in reference
	vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}

void VulkanRenderer::getPhysicalDevice()
{ 
	//get number of available physical devices (GPUs)
	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr);

	if(physDeviceCount == 0)
	{
		throw std::runtime_error("Cant find GPUs that support Vulkan instance");
	}

	//get list of physical devices
	std::vector<VkPhysicalDevice> deviceList(physDeviceCount);
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, deviceList.data());

	for (const auto& device : deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			break;
		}
	}
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* extensionsToCheck)
{
	//need to get the number of extensions to create array that has the correct size to hold the extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	//create list of VkExtensionProperties using the number of extensions we just queried
	std::vector<VkExtensionProperties> extensions(extensionCount);
	//fill it with all the VkExtensionProperties
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	//check if the given extensions are in list of available extensions
	for (const auto &extensionToCheck : *extensionsToCheck)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(extensionToCheck, extension.extensionName))
			{
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension)
		{
			return false;
		}
	}
	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	
	//VkPhysicalDeviceProperties deviceProperties;
	//vkGetPhysicalDeviceProperties(device, &deviceProperties);

	//VkPhysicalDeviceFeatures deviceFeatures;
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	
	QueueFamilyIndices queueIndices = getQueueFamilyIndices(device);

	return queueIndices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilyIndices(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	//check if each family has at least 1 of the required types of queue
	int i = 0;
	for (const auto& queueFamily : queueFamilyList)
	{
		//check if theres at least one queue and its of type Graphics Queue
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			//if queue family is what were looking for save the index in the struct
			indices.graphicsFamily = i;
		}

		if (indices.isValid()) {
			break;
		}

		i++;
	}
	return indices;
}

