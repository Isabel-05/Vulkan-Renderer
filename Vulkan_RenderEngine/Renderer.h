#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/include/glfw3.h>

#include <stdexcept>
#include <vector>

class Vertex;

class VulkanRenderer
{
public:
	//interface Functions

	VulkanRenderer();
	~VulkanRenderer();

	int init(GLFWwindow* newWindow);
	void cleanup();

	void drawFrame();
	void updateUniformBuffer(uint32_t currentImage);

	//public Components

	bool framebufferResized = false;

private:

	const int MAX_FRAMES_IN_FLIGHT = 2;	
	uint32_t currentFrame = 0;

	VkPipeline graphicsPipeline;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;



	void createRenderPass();
	void createDescriptorSetLayout();
	void createDescriptorPool();
	void createDescriptorSets();

	void createGraphicsPipeline();
	void createVertexBuffer();
	void createIndexBuffer();

	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};

