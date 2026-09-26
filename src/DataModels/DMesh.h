#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>

#include "Modifiers.h"


struct MeshVersion
{
	uint32_t topology = 1;
	uint32_t positions = 1;
	uint32_t normals = 1;
	uint32_t uvs = 1;
	uint32_t selection = 1;

	bool operator==(const MeshVersion& inVersion) const
	{
		return topology == inVersion.topology && positions == inVersion.positions &&
			normals == inVersion.normals && uvs == inVersion.uvs && selection == inVersion.selection;
	}

	bool operator!=(const MeshVersion& inVersion) const { return !(*this == inVersion); }
};

class DMesh
{

public:
	DMesh() = default;
	void init(uint32_t _id, std::string mPath);
	void loadObj(const std::string& path);

	uint32_t id;
	MeshVersion version = {};
	
	//mesh data
	std::vector<glm::vec3> positions;
	std::vector<uint8_t> vertSelected;

	std::vector<glm::vec2> edges;

	std::vector<uint32_t> faceOffsets;
	std::vector<uint8_t> sharpFaces;

	std::vector<uint32_t> cornerVerts;
	std::vector<glm::vec2> cornerUv;
	//CHECK IF NEEDED
	std::vector<uint32_t>  cornerEdges;

	//modifier stack
	std::vector<MeshModifier> modifiers;


	////////////////////
	// HELPER FUNCTIONS

	void buildEdgesFromFaces();

	uint32_t getFaceCount() const { return faceOffsets.empty() ? 0 : (uint32_t)faceOffsets.size() - 1; }
	uint32_t getFaceSize(uint32_t f) const { return faceOffsets[f + 1] - faceOffsets[f]; }

	void markPositionsDirty() { version.positions++; version.normals++; }
	void markTopologyDirty() { version.topology++;  version.normals++; version.uvs++; version.selection++; }
	void markShadingDirty() { version.normals++; }
	void markSelectionDirty() { version.selection++; }
};



