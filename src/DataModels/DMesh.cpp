#include "DMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>


void DMesh::init(uint32_t _id, std::string mPath)
{
    id = _id;
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
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str()), nullptr, false) {
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

	sharpFaces.assign(getFaceCount(), 0);
	vertSelected.assign(positions.size(), 0);

	buildEdgesFromFaces();
}

void DMesh::buildEdgesFromFaces()
{
	edges.clear();
	cornerEdges.assign(cornerVerts.size(), 0);

	std::unordered_map<uint64_t, uint32_t> edgeLookup;
	edgeLookup.reserve(cornerVerts.size());

	auto keyOf = [](uint32_t a, uint32_t b) -> uint64_t
		{
			uint32_t lo = std::min(a, b), hi = std::max(a, b);
			return (uint64_t(lo) << 32) | uint64_t(hi);
		};

	for (uint32_t f = 0; f < getFaceCount(); f++)
	{
		uint32_t start = faceOffsets[f];
		uint32_t n = getFaceSize(f);

		for (uint32_t i = 0; i < n; i++)
		{
			uint32_t c0 = start + i;
			uint32_t c1 = start + (i + 1) % n;
			uint32_t v0 = cornerVerts[c0];
			uint32_t v1 = cornerVerts[c1];

			uint64_t key = keyOf(v0, v1);
			auto it = edgeLookup.find(key);
			uint32_t edgeIdx;
			if (it == edgeLookup.end())
			{
				edgeIdx = (uint32_t)edges.size();
				edges.push_back({ std::min(v0, v1), std::max(v0, v1) });
				edgeLookup.emplace(key, edgeIdx);
			}
			else
			{
				edgeIdx = it->second;
			}
			cornerEdges[c0] = edgeIdx;
		}
	}
}
