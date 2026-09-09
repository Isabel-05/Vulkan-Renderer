#pragma once
#include <vector>
#include <string>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;
	glm::vec3 normal;

	bool operator==(const Vertex& other) const;
};


class DMesh
{

public:
	DMesh();
	void init(std::string mPath);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	void loadObj(const std::string& path);

	bool isDirty = true;
private:

	std::string modelPath;
};



