#include "DMesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

DMesh::DMesh()
{

}

void DMesh::init(std::string mPath)
{
	modelPath = mPath;

	loadObj(modelPath);
}

namespace std {
    template<typename T>
    void hashCombine(size_t& seed, const T& v) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<>
    struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            size_t seed = 0;
            hashCombine(seed, vertex.position);
            hashCombine(seed, vertex.color);
            hashCombine(seed, vertex.normal);
            hashCombine(seed, vertex.texCoord);
            return seed;
        }
    };
}

void DMesh::loadObj(const std::string& path)
{
	vertices.clear();
	indices.clear();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;
	std::string warn;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str())) {
		throw std::runtime_error(err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 0.0f };
            vertex.normal = { 0.0f, 0.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}
