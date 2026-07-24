#pragma once
#include "Renderer.h"
#include "ImGuiRenderer.h"
#include <chrono>
#include <iostream>

const std::string MODEL_PATH = "C:/Users/Administrator/Documents/Projects/Graphics Programming/repos/Vulkan_RenderEngine/models/Wolf_student_girl.obj";
const std::string TEXTURE_PATH = "C:/Users/Administrator/Documents/Projects/Graphics Programming/repos/Vulkan_RenderEngine/textures/WolfGirl_Base.png";

//const std::string MODEL_PATH = "C:/Users/Administrator/Documents/Projects/Graphics Programming/repos/Vulkan_RenderEngine/models/viking_room.obj";
//const std::string TEXTURE_PATH = "C:/Users/Administrator/Documents/Projects/Graphics Programming/repos/Vulkan_RenderEngine/textures/viking_room.png";

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	try {

		camera = Camera();
		context.init(newWindow);
		swapChain.createSwapchain(context);
		swapChain.createImageViews(context);
		frameData.createDescriptorSetLayout(context);
		graphicsPipeline.create(context, swapChain.imageFormat, frameData.descriptorSetLayout);
		commandPool.create(context);
		ModelUtil::loadObjFile(MODEL_PATH, testObject.mesh.vertices, testObject.mesh.indices);
		swapChain.createDepthResources(context, commandPool);
		ImageUtils::createTextureImage(context, commandPool, TEXTURE_PATH, testObject.material.textures, testObject.material.textureMemories);
		ImageUtils::createImageView(context, testObject.material.textures, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, testObject.material.textureImageViews);
		ImageUtils::createImageSampler(context, testObject.material.textureSampler);
		createVertexBuffer();
		createIndexBuffer();
		frameData.createUniformBuffers(context);
		frameData.createDescriptorPool(context);
		frameData.createDescriptorSets(context, testObject.material.textureImageViews, testObject.material.textureSampler);
		frameData.createCommandBuffers(context, commandPool);
		frameData.createSyncObjects(context, swapChain.imageCount);

		guiRenderer = new ImGuiRenderer(context, frameData.maxFramesInFlight);
		(*guiRenderer).init((float)swapChain.extent.width, (float)swapChain.extent.height);
		(*guiRenderer).initResources();
	}
	catch (const std::runtime_error& e)
	{
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return 0;
}

void VulkanRenderer::cleanup()
{
	(*guiRenderer).cleanup();

	swapChain.cleanupSwapChain(context);

	testObject.cleanup(context);

	vkDestroyBuffer(context.logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, indexBufferMemory, nullptr);

	vkDestroyBuffer(context.logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, vertexBufferMemory, nullptr);

	graphicsPipeline.cleanup(context);

	renderPass.cleanup(context);

	frameData.cleanup(context, swapChain.imageCount);

	commandPool.cleanup(context);

	context.cleanup();

	delete guiRenderer;
}

void VulkanRenderer::drawFrame(glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	vkWaitForFences(context.logicalDevice, 1, &frameData.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(context.logicalDevice, swapChain.handle, UINT64_MAX, frameData.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//Check if swap chain is out of date (e.g. window resized) and needs to be recreated
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		swapChain.recreateSwapChain(context, commandPool, renderPass.handle);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	updateUniformBuffer(currentFrame, viewMatrix, projectionMatrix);

	vkResetFences(context.logicalDevice, 1, &frameData.inFlightFences[currentFrame]);

	vkResetCommandBuffer(frameData.commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

	recordCommandBuffer(frameData.commandBuffers[currentFrame], imageIndex);

	//ImGui rendering Start
	(*guiRenderer).newFrame();
	(*guiRenderer).updateBuffers(currentFrame, frameData.maxFramesInFlight);
	(*guiRenderer).recordCmdBuffer(currentFrame, frameData.commandBuffers[currentFrame], commandPool, swapChain.imageViews[imageIndex]);
	//ImGui rendering End

	ImageUtils::transitionImageLayout(context, frameData.commandBuffers[currentFrame], swapChain.images[imageIndex], swapChain.imageFormat, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//finish recording command buffer
	if (vkEndCommandBuffer(frameData.commandBuffers[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { frameData.imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frameData.commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { frameData.renderFinishedSemaphores[imageIndex] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, frameData.inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.handle };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		swapChain.recreateSwapChain(context, commandPool, renderPass.handle);
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % frameData.maxFramesInFlight;
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage, glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	//static auto startTime = std::chrono::high_resolution_clock::now();

	//auto currentTime = std::chrono::high_resolution_clock::now();
	//float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	//UniformBufferObject ubo{};
	//ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	//ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.extent.width / (float)swapChain.extent.height, 0.1f, 10.0f);
	////different coordinate system in vulkan than opengl, so we flip the y coordinate of the clip space in the projection matrix

	UniformBufferObject ubo{};
	ubo.model = glm::mat4(1.0f);
	ubo.view = viewMatrix;
	ubo.proj = projectionMatrix;
	ubo.proj[1][1] *= -1;

	memcpy(frameData.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	//Begin recording
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional (specifies how were using the buffer)
	beginInfo.pInheritanceInfo = nullptr; // Optional (only needed for secondary buffers)

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	ImageUtils::transitionImageLayout(context, commandBuffer, swapChain.images[imageIndex], swapChain.imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfoKHR colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	colorAttachment.imageView = swapChain.imageViews[imageIndex];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderingAttachmentInfo depthInfo{};
	depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthInfo.pNext = nullptr;
	depthInfo.imageView = swapChain.depthImageView;
	depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthInfo.resolveMode = VK_RESOLVE_MODE_NONE;
	depthInfo.resolveImageView = VK_NULL_HANDLE;
	depthInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthInfo.clearValue.depthStencil = { 1.0f, 0 };

	VkRenderingInfoKHR renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.renderArea = { {0, 0}, swapChain.extent };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthInfo;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	//bind graphics pipeline
	//second parameter decides if its a graphics or compute pipeline
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.handle);

	VkBuffer vertexBuffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	//set our dynamic viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChain.extent.width);
	viewport.height = static_cast<float>(swapChain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain.extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	//bind descriptor sets (for passing uniform buffer data to shaders)
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout, 0, 1, &frameData.descriptorSets[currentFrame], 0, nullptr);

	//Draw command
	//parameter 3: vertex count
	//parameter 4: instanceCount: Used for instanced rendering, use 1 if you're not doing that.
	//parameter 5: firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
	//parameter 6: firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(testObject.mesh.indices.size()), 1, 0, 0, 0);

	vkCmdEndRendering(commandBuffer);
}

void VulkanRenderer::createVertexBuffer()
{
	
	VkDeviceSize bufferSize = sizeof(testObject.mesh.vertices[0]) * testObject.mesh.vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	//VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
	//VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.
	BufferUtils::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(context.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, testObject.mesh.vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(context.logicalDevice, stagingBufferMemory);

	//specify that we only want to use the vertex buffer as destination for transfer and as vertex buffer
	BufferUtils::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	BufferUtils::copyBuffer(context, commandPool, stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(context.logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, stagingBufferMemory, nullptr);
}

void VulkanRenderer::createIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(testObject.mesh.indices[0]) * testObject.mesh.indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	BufferUtils::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(context.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, testObject.mesh.indices.data(), (size_t)bufferSize);
	vkUnmapMemory(context.logicalDevice, stagingBufferMemory);

	BufferUtils::createBuffer(context, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	BufferUtils::copyBuffer(context, commandPool, stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(context.logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(context.logicalDevice, stagingBufferMemory, nullptr);
}
