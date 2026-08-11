#include "Renderer.h"
#include "ImGuiRenderer.h"
#include <chrono>
#include <iostream>

//const std::string MODEL_PATH = std::string(ASSET_DIR) + "models/viking_room.obj";
//const std::string TEXTURE_PATH = std::string(ASSET_DIR) + "textures/viking_room.png";
//const std::string MODEL_PATH2 = std::string(ASSET_DIR) + "models/Wolf_student_girl.obj";
//const std::string TEXTURE_PATH2 = std::string(ASSET_DIR) + "textures/WolfGirl_Base.png";

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	try {

		camera = Camera();
		context.init(newWindow);

		int winW, fbW;
		glfwGetWindowSize(newWindow, &winW, nullptr);
		glfwGetFramebufferSize(newWindow, &fbW, nullptr);
		inputScale = abs((float)fbW / winW);

		//Base Vulkan setup
		swapChain.createSwapchain(context);
		swapChain.createImageViews(context);
		frameData.createDescriptorSetLayouts(context);
		graphicsPipeline.create(context, swapChain.imageFormat, frameData.cameraDSLayout, frameData.materialDSLayout);
		commandPool.create(context);
		swapChain.createColorResources(context, commandPool);
		swapChain.createDepthResources(context, commandPool);
		swapChain.createOutputResources(context, frameData.maxFramesInFlight);

		//Rendering loop resources
		frameData.createUniformBuffers(context);
		frameData.createDescriptorPool(context);
		frameData.createDescriptorSets(context);
		frameData.createCommandBuffers(context, commandPool);
		frameData.createSyncObjects(context, swapChain.imageCount);

		//ImGui setup
		guiRenderer = std::make_unique<ImGuiRenderer>(ImGuiRenderer(context, frameData.maxFramesInFlight));
		guiRenderer->init((float)swapChain.extent.width, (float)swapChain.extent.height);
		guiRenderer->loadOutputImages(swapChain.outputSampler, swapChain.outputImageViews);
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
	vkDeviceWaitIdle(context.logicalDevice);

	guiRenderer->cleanup();

	swapChain.cleanupSwapChain(context);

	scene.cleanup(context);

	graphicsPipeline.cleanup(context);

	frameData.cleanup(context, swapChain.imageCount);

	commandPool.cleanup(context);

	context.cleanup();
}


void VulkanRenderer::drawFrame()
{
	glm::mat4 viewMatrix = camera.getViewMatrix();
	glm::mat4 projectionMatrix = camera.getProjectionMatrix((float)swapChain.extent.width / (float)swapChain.extent.height, 0.1f, 20.0f);

	vkWaitForFences(context.logicalDevice, 1, &frameData.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(context.logicalDevice, swapChain.handle, UINT64_MAX, frameData.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//Check if swap chain is out of date (e.g. window resized) and needs to be recreated
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		swapChain.recreateSwapChain(context, commandPool, frameData.maxFramesInFlight);
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	guiRenderer->newFrame(commandPool, currentFrame, scene, frameData.descriptorPool, frameData.materialDSLayout);

	updateUniformBuffer(currentFrame, viewMatrix, projectionMatrix);

	vkResetFences(context.logicalDevice, 1, &frameData.inFlightFences[currentFrame]);

	vkResetCommandBuffer(frameData.commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);

	recordCommandBuffer(frameData.commandBuffers[currentFrame], imageIndex);

	//ImGui rendering Start

	guiRenderer->updateBuffers(currentFrame, frameData.maxFramesInFlight);
	ImageUtils::transitionImageLayout(context, frameData.commandBuffers[currentFrame], swapChain.images[imageIndex], swapChain.imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	guiRenderer->recordCmdBuffer(currentFrame, frameData.commandBuffers[currentFrame], commandPool, swapChain.imageViews[imageIndex]);
	//ImGui rendering End

	ImageUtils::transitionImageLayout(context, frameData.commandBuffers[currentFrame], swapChain.images[imageIndex], swapChain.imageFormat,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

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

	VkResult submitResult = vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, frameData.inFlightFences[currentFrame]);
	if (submitResult != VK_SUCCESS) {
		std::cerr << "vkQueueSubmit failed with VkResult: " << submitResult << std::endl;
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
		swapChain.recreateSwapChain(context, commandPool, frameData.maxFramesInFlight);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(swapChain.extent.width), static_cast<float>(swapChain.extent.height));
		guiRenderer->reloadOutputImages(swapChain.outputSampler, swapChain.outputImageViews);
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	currentFrame = (currentFrame + 1) % frameData.maxFramesInFlight;
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

	VkImageMemoryBarrier2 toColorAttachment{};
	toColorAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	toColorAttachment.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	toColorAttachment.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT; // 0 if first use
	toColorAttachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	toColorAttachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	toColorAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // or SHADER_READ_ONLY_OPTIMAL
	toColorAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	toColorAttachment.image = swapChain.outputImages[currentFrame];
	toColorAttachment.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dep.imageMemoryBarrierCount = 1;
	dep.pImageMemoryBarriers = &toColorAttachment;
	vkCmdPipelineBarrier2(commandBuffer, &dep);

	VkRenderingAttachmentInfoKHR colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	colorAttachment.imageView = swapChain.colorImageView;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	colorAttachment.resolveImageView = swapChain.outputImageViews[currentFrame];
	colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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
	//VkExtent2D imageExtent = { guiRenderer->getViewportSize().x, guiRenderer->getViewportSize().y };
	scissor.extent = swapChain.extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	for (auto& obj : scene.objList)
	{
		//if (obj.mesh.indices.empty() || obj.mesh.vertices.empty()) continue;
		VkBuffer vertexBuffers[] = { obj.mesh.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, obj.mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		//bind descriptor sets (for passing uniform buffer data to shaders)
		VkDescriptorSet sets[] = { frameData.cameraDescriptorSets[currentFrame], obj.material.descriptorSet };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.pipelineLayout, 0, 2, sets, 0, nullptr);

		//push constants (for passing model matrix to vertex shader)
		glm::mat4 modelMatrix = obj.getModelMatrix();
		vkCmdPushConstants(commandBuffer, graphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

		//Draw command
		//parameter 3: vertex count
		//parameter 4: instanceCount: Used for instanced rendering, use 1 if you're not doing that.
		//parameter 5: firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
		//parameter 6: firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj.mesh.indices.size()), 1, 0, 0, 0);
	}

	vkCmdEndRendering(commandBuffer);

	VkImageMemoryBarrier2 toShaderRead{};
	toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	toShaderRead.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	toShaderRead.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	toShaderRead.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	toShaderRead.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	toShaderRead.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	toShaderRead.image = swapChain.outputImages[currentFrame];
	toShaderRead.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &toShaderRead;
	vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage, glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	UniformBufferObject ubo{};
	ubo.view = viewMatrix;
	ubo.proj = projectionMatrix;
	ubo.proj[1][1] *= -1;

	memcpy(frameData.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}


/////////////
//PUBLIC API

void VulkanRenderer::onResize()
{
	framebufferResized = true;
}

void VulkanRenderer::onKey(int key, int scancode, int action, int mods)
{
	guiRenderer->handleKey(key, scancode, action, mods);
}

void VulkanRenderer::onMouseMove(double xpos, double ypos, float xoffset, float yoffset)
{
	camera.processMouseMovement(xoffset, yoffset);
	guiRenderer->handleMousePos(static_cast<float>(xpos * inputScale), static_cast<float>(ypos * inputScale));
}

void VulkanRenderer::onMousePressed(int button, int action, int mods)
{
	if (!guiRenderer->isViewportHovered())
	{
		guiRenderer->handleMouseButton(button, action); // Pass mouse button state to ImGui for UI interaction

		// if the mouse leaves the viewport while the mouse is pressed release the camera
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		{
			camera.mousePressed = false;
		}
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		camera.mousePressed = true;  // Set flag to indicate left mouse button is pressed
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		camera.mousePressed = false; // Clear flag when left mouse button is released
	}

	//if mouse leaves window while ui is being pressed set to released
	guiRenderer->handleMouseButton(button, action);
}

void VulkanRenderer::onMouseWheel(double xoffset, double yoffset)
{
	if (ImGui::GetIO().WantCaptureMouse)
	{
		// Pass mouse wheel input to ImGui for UI interaction
		ImGuiIO& io = ImGui::GetIO();
		io.AddMouseWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));
		return;
	}
}

