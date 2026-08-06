#pragma once
#include "RenderObject.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include "BufferUtils.h"
#include "Image.h"


void RenderObject::init(VulkanContext& context, CommandPool& cmdPool, std::string modelPath, std::string texturePath,
	VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
{
	ModelUtil::loadObjFile(modelPath, mesh.vertices, mesh.indices);
	scale = glm::vec3(1.0f, 1.0f, 1.0f);
	position = glm::vec3(0.0f, 0.0f, 1.0f);
	rotation = glm::vec3(0.0f, 0.0f, 0.0f);

	ImageUtils::createTextureImage(context, cmdPool, texturePath, material.texture, material.textureMemory, material.mipLevels);
	ImageUtils::createImageView(context, material.texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, material.textureImageView, material.mipLevels);
	ImageUtils::createImageSampler(context, material.textureSampler);
	material.createDescriptorSets(context, pool, descriptorSetLayout);
	

	mesh.upload(context, cmdPool);
}

glm::mat4 RenderObject::getModelMatrix() const
{
	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
	modelMatrix = glm::scale(modelMatrix, scale);
	return modelMatrix;
}

void Mesh::upload(VulkanContext& context, CommandPool& cmdPool)
{
	BufferUtils::uploadBufferToGpu<Vertex>(context, cmdPool, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, vertexBuffer, vertexBufferMemory);
	BufferUtils::uploadBufferToGpu<uint32_t>(context, cmdPool, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices, indexBuffer, indexBufferMemory);
}

void RenderObject::cleanup(VulkanContext& context)
{
	material.cleanup(context);
	mesh.cleanup(context);
}

void Mesh::cleanup(VulkanContext& context)
{
	vkDestroyBuffer(context.logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, indexBufferMemory, nullptr);
	vkDestroyBuffer(context.logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, vertexBufferMemory, nullptr);
}

void Material::createDescriptorSets(VulkanContext& context, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
{
	std::vector<VkDescriptorSetLayout> layout(1, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layout.data();

	if (vkAllocateDescriptorSets(context.logicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = textureImageView;
	imageInfo.sampler = textureSampler;

	VkWriteDescriptorSet descriptorWrites{};

	descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites.dstSet = descriptorSet;
	descriptorWrites.dstBinding = 1;
	descriptorWrites.dstArrayElement = 0;
	descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites.descriptorCount = 1;
	descriptorWrites.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(context.logicalDevice, 1, &descriptorWrites, 0, nullptr);

}

void Material::cleanup(VulkanContext& context)
{
	vkDestroySampler(context.logicalDevice, textureSampler, nullptr);
	vkDestroyImageView(context.logicalDevice, textureImageView, nullptr);

	vkDestroyImage(context.logicalDevice, texture, nullptr);
	vkFreeMemory(context.logicalDevice, textureMemory, nullptr);
}


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
	}
}