#include "DMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>


void DMesh::init(uint32_t _id, std::string mPath)
{
    id = _id;

	scale = glm::vec3(1.0f, 1.0f, 1.0f);
	position = glm::vec3(0.0f, 0.0f, 0.0f);
	rotation = glm::vec3(0.0f, 0.0f, 0.0f);

	loadObj(mPath);
}

void DMesh::loadObj(const std::string& path)
{
	positions.clear();
    vertSelected.clear();
	edges.clear();
	faceOffsets.clear();
	sharpFaces.clear();
	cornerVerts.clear();
	cornerUv.clear();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

    //triangulate false
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(), nullptr, false)) {
		throw std::runtime_error(err);
	}

	//copy vertex positions
    positions.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i < attrib.vertices.size(); i += 3)
        positions.push_back({ attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2] });


	//fill corners and faceOffsets (and build edges from that)
    faceOffsets.push_back(0);
	for (const auto& shape : shapes) {
        size_t idxOffset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
			int fv = shape.mesh.num_face_vertices[f];
			for (int c = 0; c < fv; c++)
			{
				const auto& idx = shape.mesh.indices[idxOffset + c];
				cornerVerts.push_back(idx.vertex_index);

				glm::vec2 uv(0.0f);
				if (idx.texcoord_index >= 0)
				{
					uv = { attrib.texcoords[2 * idx.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * idx.texcoord_index + 1] };
				}
				cornerUv.push_back(uv);
			}
			faceOffsets.push_back((uint32_t)cornerVerts.size());
			idxOffset += fv;
		}
	}

	sharpFaces.assign(getFaceCount(), 1);
	vertSelected.assign(positions.size(), 0);

	buildEdgesFromFaces();
}


uint64_t keyOf(uint32_t a, uint32_t b)
{
	uint32_t low = std::min(a, b), high = std::max(a, b);
	return (uint64_t(low) << 32) | uint64_t(high);
}

void DMesh::buildEdgesFromFaces()
{
	edges.clear();

	//lookup table to not add duplicated edges
	std::unordered_map<uint64_t, uint32_t> edgeLookup;
	edgeLookup.reserve(cornerVerts.size());

	//loop over all faces in the mesh
	for (uint32_t faceId = 0; faceId < getFaceCount(); faceId++)
	{
		uint32_t currFace = faceOffsets[faceId];
		uint32_t numIdx = getFaceSize(faceId);

		//loop through all indices of the face
		for (uint32_t idx = 0; idx < numIdx; idx++)
		{
			//get the next two vertices (ie the next edge)
			uint32_t v0 = cornerVerts[currFace + idx];
			uint32_t v1 = cornerVerts[currFace + (idx + 1) % numIdx];

			//make the edge key and check if it already exists
			uint64_t key = keyOf(v0, v1);
			auto it = edgeLookup.find(key);

			//if the edge doesnt exist yet, add it to the edge list and the lookup table
			if (it == edgeLookup.end())
			{
				edges.push_back({ std::min(v0, v1), std::max(v0, v1) });
				edgeLookup.emplace(key, (uint32_t)edges.size());
			}
		}
	}
}

void DMesh::clearSelection()
{
	std::fill(vertSelected.begin(), vertSelected.end(), 0);
}

std::vector<uint32_t> DMesh::getSelectedVertIds()
{
	std::vector<uint32_t> out;
	for (uint32_t i = 0; i < vertSelected.size(); i++)
	{
		if (vertSelected[i] == 1)
			out.push_back(i);
	}
	return out;
}