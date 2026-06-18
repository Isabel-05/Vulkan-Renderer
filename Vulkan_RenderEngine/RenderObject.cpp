#pragma once
#include "RenderObject.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

namespace ModelUtil
{
	void loadObjFile(std::string filePath, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		std::string warn;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str())) {
			throw std::runtime_error(err);
		}

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };

				vertices.push_back(vertex);
				indices.push_back(indices.size());
			}
		}

		for (auto& vertex : vertices) {
			vertex.pos = glm::vec4(vertex.pos, 0) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			vertex.pos.z -= 1.5f;
		}
	}
}

void Material::cleanup(VulkanContext& context)
{
	vkDestroySampler(context.logicalDevice, textureSampler, nullptr);
	vkDestroyImageView(context.logicalDevice, textureImageViews, nullptr);

	vkDestroyImage(context.logicalDevice, textures, nullptr);
	vkFreeMemory(context.logicalDevice, textureMemories, nullptr);
}

void RenderObject::cleanup(VulkanContext& context)
{
	material.cleanup(context);
}
