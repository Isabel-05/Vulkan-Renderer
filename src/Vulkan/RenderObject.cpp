#include "RenderObject.h"

void RenderObject::init(VulkanContext& context, CommandPool& cmdPool, std::shared_ptr<DMesh> dmesh)
{
	gpuCache.init(context, cmdPool, dmesh);
}

glm::mat4 RenderObject::getModelMatrix() const
{
	DMesh dmesh = *gpuCache.dataMesh;
	glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), dmesh.position);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(dmesh.rotation.x), glm::vec3(1, 0, 0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(dmesh.rotation.y), glm::vec3(0, 1, 0));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(dmesh.rotation.z), glm::vec3(0, 0, 1));
	modelMatrix = glm::scale(modelMatrix, dmesh.scale);
	return modelMatrix;
}

void RenderObject::sync(VulkanContext& context, CommandPool& cmdPool, bool wantOverlay)
{
	gpuCache.sync(context, cmdPool, wantOverlay);
}

void RenderObject::drawSurface(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS)
{
	if (gpuCache.surfaceIdxBuffer.count == 0) return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *surfaceMaterial.pipeline);

	VkBuffer vertexBuffers[] = { gpuCache.surfaceVertBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, gpuCache.surfaceIdxBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	VkDescriptorSet sets[] = { cameraDS };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *surfaceMaterial.pipelineLayout, 0, 1, sets, 0, nullptr);

	glm::mat4 modelMatrix = getModelMatrix();
	vkCmdPushConstants(commandBuffer, *surfaceMaterial.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

	vkCmdDrawIndexed(commandBuffer, gpuCache.surfaceIdxBuffer.count, 1, 0, 0, 0);
}

void RenderObject::drawEdges(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS)
{
	if (gpuCache.edgeIdxBuffer.count == 0 || !edgeMaterial.pipeline) return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *edgeMaterial.pipeline);

	VkBuffer vertexBuffers[] = { gpuCache.pointBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, gpuCache.edgeIdxBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

	VkDescriptorSet sets[] = { cameraDS };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *edgeMaterial.pipelineLayout, 0, 1, sets, 0, nullptr);

	glm::mat4 modelMatrix = getModelMatrix();
	vkCmdPushConstants(commandBuffer, *edgeMaterial.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

	vkCmdDrawIndexed(commandBuffer, gpuCache.edgeIdxBuffer.count, 1, 0, 0, 0);
}

void RenderObject::drawPoints(VulkanContext& context, CommandPool& cmdPool, VkCommandBuffer& commandBuffer, VkDescriptorSet& cameraDS)
{
	if (gpuCache.pointBuffer.count == 0 || !pointMaterial.pipeline) return;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pointMaterial.pipeline);

	VkBuffer vertexBuffers[] = { gpuCache.pointBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	VkDescriptorSet sets[] = { cameraDS };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pointMaterial.pipelineLayout, 0, 1, sets, 0, nullptr);

	glm::mat4 modelMatrix = getModelMatrix();
	vkCmdPushConstants(commandBuffer, *pointMaterial.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

	vkCmdDraw(commandBuffer, gpuCache.pointBuffer.count, 1, 0, 0);
}

void RenderObject::cleanup(VulkanContext& context)
{
	gpuCache.cleanup(context);
}


