#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/include/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Utilities.h"

class VulkanRenderer
{
public:
	//interface Functions

	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void cleanup();

	void drawFrame();

	//public Components
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	}
	mainDevice;

private:

	const int MAX_FRAMES_IN_FLIGHT = 2;	
	uint32_t currentFrame = 0;

	GLFWwindow* window;

	//Vulkan Components
	VkInstance instance;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkSurfaceKHR surface;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	uint32_t swapChainImageCount;

	VkPipeline graphicsPipeline;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};


	//Vulkan Functions

	//createFunctions
	void createInstance();
	void getPhysicalDevice();
	void createLogicalDevice();
	void createSurface();
	void createSwapchain();
	void createImageViews();
	void createRenderPass();

	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSyncObjects();

	//support Functions
	bool checkInstanceExtensionSupport(std::vector<const char*> *extensionsToCheck);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	bool checkValidationLayerSupport();

	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	//swap chain choose functions
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};

