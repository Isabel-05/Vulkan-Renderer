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


void GpuMaterial::init()
{

}


////////////////
//RENDER OBJECT

void RenderObject::init(VulkanContext& context, CommandPool& cmdPool, DMesh& dmesh)
{

	mesh.init(context, cmdPool, dmesh);

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

void RenderObject::checkAndUpdateMesh(VulkanContext& context, CommandPool& cmdPool)
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
}

void RenderObject::draw(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS)
{
		
	checkAndUpdateMesh(context, cmdPool);

	for (auto& material : materials)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *material.pipeline);

		VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		//bind descriptor sets (for passing uniform buffer data to shaders)
		VkDescriptorSet sets[] = { cameraDS };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *material.pipelineLayout, 0, 1, sets, 0, nullptr);

		//push constants (for passing model matrix to vertex shader)
		glm::mat4 modelMatrix = getModelMatrix();
		vkCmdPushConstants(commandBuffer, *material.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

		//Draw call
		//parameter 3: vertex count
		//parameter 4: instanceCount: Used for instanced rendering, use 1 if you're not doing that.
		//parameter 5: firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
		//parameter 6: firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
		vkCmdDrawIndexed(commandBuffer, mesh.indexCount, 1, 0, 0, 0);
	}
	
}

void RenderObject::cleanup(VulkanContext& context)
{
	mesh.cleanup(context);
}

