#pragma once
#include "Vertex.h"
#include <vector>
#include <string>

namespace ModelUtil
{
	void loadObjFile(std::string filePath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
}

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct Material
{
	std::vector<VkImage> textures;
	std::string shaderPath;
};

class RenderObject
{
	Mesh mesh;
	Material material;
	glm::mat4 transform;
};

