#pragma once
#include <vector>
#include <string>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Modifiers.h"
#include "Vertex.h"


class DMesh
{

public:
	DMesh();
	void init(std::string mPath);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<std::unique_ptr<MeshModifier>> modifiers;

	void loadObj(const std::string& path);

	bool isDirty = true;
private:

	std::string modelPath;
};



