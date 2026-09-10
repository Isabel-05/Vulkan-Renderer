#include "RenderObject.h"
#include "BufferUtils.h"
#include "Image.h"

///////////////
//MESH

void GpuMesh::init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh)
{
	dataMesh = std::make_shared<DMesh>(dmesh);
	upload(context, cmdPool, dataMesh->vertices, dataMesh->indices);
}

void GpuMesh::upload(VulkanContext& context, CommandPool& cmdPool, std::vector<Vertex> verts, std::vector<uint32_t> indices)
{
	BufferUtils::uploadBufferToGpu<Vertex>(context, cmdPool, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, verts, vertexBuffer, vertexBufferMemory);
	BufferUtils::uploadBufferToGpu<uint32_t>(context, cmdPool, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices, indexBuffer, indexBufferMemory);
	indexCount = static_cast<uint32_t>(indices.size());
}

void GpuMesh::cleanup(VulkanContext& context)
{
	vkDestroyBuffer(context.logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, indexBufferMemory, nullptr);
	vkDestroyBuffer(context.logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, vertexBufferMemory, nullptr);
}

/////////////////
//MATERIAL


void GpuMaterial::init(VulkanContext& context, CommandPool& cmdPool, DMaterial& dmaterial, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
{
	dataMaterial = std::make_shared<DMaterial>(dmaterial);
	ImageUtils::createImageSampler(context, textureSampler);
	initTexResources(context, cmdPool, pool, descriptorSetLayout);
	createDescriptorSets(context, pool, descriptorSetLayout);
}

void GpuMaterial::initTexResources(VulkanContext& context, CommandPool& cmdPool, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
{
	ImageUtils::createTextureImage(context, cmdPool, dataMaterial->texturePaths[0], texture, textureMemory, mipLevels);
	ImageUtils::createImageView(context, texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, textureImageView, mipLevels);
}

void GpuMaterial::createDescriptorSets(VulkanContext& context, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
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

void GpuMaterial::updateDescriptorSets(VulkanContext& context, VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
{
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

void GpuMaterial::cleanupTexResources(VulkanContext& context)
{
	vkDestroyImageView(context.logicalDevice, textureImageView, nullptr);
	vkDestroyImage(context.logicalDevice, texture, nullptr);
	vkFreeMemory(context.logicalDevice, textureMemory, nullptr);
}

void GpuMaterial::cleanup(VulkanContext& context)
{
	vkDestroySampler(context.logicalDevice, textureSampler, nullptr);
	cleanupTexResources(context);
}


////////////////
//RENDER OBJECT

void RenderObject::init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh, DMaterial& dmaterial,
	VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
{

	mesh.init(context, cmdPool, dmesh);
	material.init(context, cmdPool, dmaterial, pool, descriptorSetLayout);

	scale = glm::vec3(1.0f, 1.0f, 1.0f);
	position = glm::vec3(0.0f, 0.0f, 1.0f);
	rotation = glm::vec3(0.0f, 0.0f, 0.0f);
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

void RenderObject::checkAndUpdateMesh(VulkanContext& context, CommandPool& cmdPool, FrameData& frameData)
{
	if (mesh.dataMesh->isDirty)
	{
		vkDeviceWaitIdle(context.logicalDevice);

		if (mesh.dataMesh->modifiers.empty())
		{
			mesh.upload(context, cmdPool, mesh.dataMesh->vertices, mesh.dataMesh->indices);
			return;
		}

		for (auto& mod : mesh.dataMesh->modifiers)
		{
			mod->evaluate(mesh.evalVertices, mesh.evalIndices);
		}

		mesh.cleanup(context);
		mesh.upload(context, cmdPool, mesh.evalVertices, mesh.evalIndices);
		mesh.dataMesh->isDirty = false;
	}
	if (material.dataMaterial->isDirty)
	{
		vkDeviceWaitIdle(context.logicalDevice);
		material.cleanupTexResources(context);
		material.initTexResources(context, cmdPool, frameData.descriptorPool, frameData.materialDSLayout);
		material.updateDescriptorSets(context, frameData.descriptorPool, frameData.materialDSLayout);
		material.dataMaterial->isDirty = false;
	}
}

void RenderObject::draw(VulkanContext& context, CommandPool& cmdPool, FrameData& frameData, VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, VkDescriptorSet& cameraDS)
{
		
	checkAndUpdateMesh(context, cmdPool, frameData);

	VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	//bind descriptor sets (for passing uniform buffer data to shaders)
	VkDescriptorSet sets[] = { cameraDS, material.descriptorSet };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, sets, 0, nullptr);

	//push constants (for passing model matrix to vertex shader)
	glm::mat4 modelMatrix = getModelMatrix();
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

	//Draw call
	//parameter 3: vertex count
	//parameter 4: instanceCount: Used for instanced rendering, use 1 if you're not doing that.
	//parameter 5: firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
	//parameter 6: firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
	vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
}

void RenderObject::cleanup(VulkanContext& context)
{
	material.cleanup(context);
	mesh.cleanup(context);
}

