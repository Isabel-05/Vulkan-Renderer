#pragma once
#include "Vertex.h"
#include <vector>
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
	std::vector<VkImage> textures;
	std::vector<VkDeviceMemory> textureMemories;	
	std::vector<VkImageView> textureImageViews;
	VkSampler textureSampler;
	std::string shaderPath;
};

class RenderObject
{
public:
	void init(std::string modelPath, std::vector<std::string> texturePaths, std::string shaderPath);
	void destroy();

	Mesh mesh;
	Material material;
	glm::mat4 transform;
};

