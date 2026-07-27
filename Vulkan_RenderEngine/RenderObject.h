#pragma once
#include "VulkanContext.h"
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
};

struct Material
{
	void cleanup(VulkanContext& context);
	VkImage textures;
	VkDeviceMemory textureMemories;	
	VkImageView textureImageViews;
	uint32_t mipLevels;
	VkSampler textureSampler;
	std::string shaderPath;
};

class RenderObject
{
public:
	void init(std::string modelPath, std::vector<std::string> texturePaths, std::string shaderPath);
	void cleanup(VulkanContext& context);

	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	Mesh mesh;
	Material material;

};

