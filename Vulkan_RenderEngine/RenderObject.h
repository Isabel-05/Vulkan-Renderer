#pragma once
#include "VulkanContext.h"
#include "CommandPool.h"

#include "Vertex.h"
#include <string>


namespace ModelUtil
{
	void loadObjFile(std::string filePath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
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
	VkImage texture;
	VkDeviceMemory textureMemory;
	VkImageView textureImageView;
	uint32_t mipLevels;
	VkSampler textureSampler;
	std::string shaderPath;

	void cleanup(VulkanContext& context);
};

class RenderObject
{
public:

	std::string name;

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	Mesh mesh;
	Material material;

	glm::mat4 getModelMatrix() const;
	void init(VulkanContext& context, CommandPool& cmdPool, std::string modelPath, std::string texturePath);
	void cleanup(VulkanContext& context);
};

