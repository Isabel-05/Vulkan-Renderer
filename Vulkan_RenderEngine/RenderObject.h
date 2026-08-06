#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

#include "Vertex.h"
#include <string>


namespace ModelUtil
{
	void loadObjFile(std::string filePath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	void loadTexture(std::string filePath, std::vector<VkImage>& textures, std::vector<VkDeviceMemory>& textureMemories, std::vector<VkImageView>& textureImageViews);
}

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void upload(VulkanContext& context, CommandPool& cmdPool);
	void cleanup(VulkanContext& context);
};

struct Material
{
	void cleanup(VulkanContext& context);
	VkImage texture;
	VkDeviceMemory textureMemory;	
	VkImageView textureImageView;
	uint32_t mipLevels;
	VkSampler textureSampler;
	std::string shaderPath;
};

class RenderObject
{
public:
	void init(VulkanContext& context, CommandPool& cmdPool, std::string modelPath, std::string texturePath);
	void cleanup(VulkanContext& context);
	glm::mat4 getModelMatrix() const;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	Mesh mesh;
	Material material;

};

