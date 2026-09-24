#include "Renderer.h"
#include "ImGuiRenderer.h"

#include <chrono>
#include <iostream>
#include <algorithm>


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
		commandPool.create(context);
		swapChain.createColorResources(context, commandPool, viewportExtent);
		swapChain.createDepthResources(context, commandPool, viewportExtent);
		swapChain.createOutputResources(context, frameData.maxFramesInFlight, viewportExtent);

		//Shader Resources
		shaderResources.createDescriptorPool(context);
		shaderResources.createDescriptorSetLayouts(context);
		shaderResources.createPipelines(context, swapChain.imageFormat);
		
		//Rendering loop resources
		frameData.createUniformBuffers(context);
		frameData.createDescriptorSets(context, shaderResources.descriptorPool, shaderResources.cameraDSLayout);
		frameData.createCommandBuffers(context, commandPool);
		frameData.createSyncObjects(context, swapChain.imageCount);

		//ImGui setup
		guiRenderer = std::make_unique<ImGuiRenderer>(ImGuiRenderer(context, frameData.maxFramesInFlight));
		guiRenderer->init((float)swapChain.extent.width, (float)swapChain.extent.height);
		guiRenderer->loadOutputImages(swapChain.outputSampler, swapChain.outputImageViews);

		idPass.createResources(context, shaderResources.cameraDSLayout, swapChain.extent);

		DMesh dmesh;
		dmesh.init(0, std::string(ASSET_DIR) + "models/BlenderCube.obj");
		FlatShadingMdf mod;
		dmesh.modifiers.push_back(std::make_shared<FlatShadingMdf>(mod));
		scene.addObj(dmesh);
		scene.setSelectedObjId(0);

		//GpuMaterial basemat;
		//basemat.pipeline = std::make_shared<VkPipeline>(shaderResources.BaseShaderPl);
		//basemat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.BaseShaderLayout);

		//GpuMaterial linemat;
		//linemat.pipeline = std::make_shared<VkPipeline>(shaderResources.LineShaderPl);
		//linemat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.LineShaderLayout);

		//GpuMaterial pointmat;
		//pointmat.pipeline = std::make_shared<VkPipeline>(shaderResources.PointShaderPl);
		//pointmat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.PointShaderLayout);
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

	for (auto& obj : renderObjects)
	{
		obj.cleanup(context);
	}

	guiRenderer->cleanup();

	swapChain.cleanup(context);

	shaderResources.cleanup(context);

	idPass.cleanup(context);

	frameData.cleanup(context, swapChain.imageCount);

	commandPool.cleanup(context);

	context.cleanup();
}

void VulkanRenderer::drawFrame()
{
	if (scene.isDirty) {
		updateObjects();
		scene.isDirty = false;
	}

	vkWaitForFences(context.logicalDevice, 1, &frameData.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(context.logicalDevice, swapChain.handle, UINT64_MAX, frameData.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	guiRenderer->newFrame(commandPool, currentFrame, scene);

	if (checkViewportResize())
		resizeViewportResources();

	glm::mat4 viewMatrix = camera.getViewMatrix();
	glm::mat4 projectionMatrix = camera.getProjectionMatrix((float)viewportExtent.width / (float)viewportExtent.height, 0.1f, 20.0f);

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

		if (checkViewportResize())
			resizeViewportResources();

		swapChain.recreateSwapChain(context, commandPool, frameData.maxFramesInFlight);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(swapChain.extent.width), static_cast<float>(swapChain.extent.height));
		
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	if (pick.wasClicked)
		updateSelection();

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
	float bgcolor = 0.05f;
	colorAttachment.clearValue.color = { bgcolor, bgcolor, bgcolor, 1.0f };

	VkRenderingAttachmentInfo depthInfo{};
	depthInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthInfo.pNext = nullptr;
	depthInfo.imageView = swapChain.depthImageView;
	depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthInfo.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	depthInfo.resolveImageView = swapChain.depthResolveView;
	depthInfo.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthInfo.clearValue.depthStencil = { 1.0f, 0 };

	VkRenderingInfoKHR renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
	renderingInfo.renderArea = { {0, 0}, viewportExtent };
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
	renderingInfo.pDepthAttachment = &depthInfo;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	//set our dynamic viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(viewportExtent.width);
	viewport.height = static_cast<float>(viewportExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	//VkExtent2D imageExtent = { guiRenderer->getViewportSize().x, guiRenderer->getViewportSize().y };
	scissor.extent = viewportExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	for (auto& RO : renderObjects)
	{
		RO.draw(context, commandPool, commandBuffer, frameData.cameraDescriptorSets[currentFrame]);
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

void VulkanRenderer::updateObjects()
{
	renderObjects.clear();

	for (auto& dmesh : scene.objList)
	{
		RenderObject newRO;
		newRO.init(context, commandPool, dmesh);
		newRO.name = dmesh.id;

		GpuMaterial basemat;
		basemat.pipeline = std::make_shared<VkPipeline>(shaderResources.BaseShaderPl);
		basemat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.BaseShaderLayout);
		newRO.materials.push_back(basemat);

		GpuMaterial linemat;
		linemat.pipeline = std::make_shared<VkPipeline>(shaderResources.LineShaderPl);
		linemat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.LineShaderLayout);
		newRO.materials.push_back(linemat);

		GpuMaterial pointmat;
		pointmat.pipeline = std::make_shared<VkPipeline>(shaderResources.PointShaderPl);
		pointmat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.PointShaderLayout);
		newRO.materials.push_back(pointmat);

		renderObjects.push_back(newRO);
	}
}

bool VulkanRenderer::checkViewportResize()
{
	ImVec2 newAvail = guiRenderer->avail;
	uint32_t newH = std::max(1, (int)newAvail.y);
	uint32_t newW = std::max(1, (int)newAvail.x);

	if ( newW != viewportExtent.width || newH != viewportExtent.height)
	{
		viewportExtent = { newW, newH };
		return true;
	}
	return false;
}

void VulkanRenderer::resizeViewportResources()
{
	vkDeviceWaitIdle(context.logicalDevice);

	swapChain.cleanupViewportImages(context);

	swapChain.createColorResources(context, commandPool, viewportExtent);
	swapChain.createDepthResources(context, commandPool, viewportExtent);
	swapChain.createOutputResources(context, frameData.maxFramesInFlight, viewportExtent);

	idPass.resize(context, viewportExtent);

	guiRenderer->reloadOutputImages(swapChain.outputSampler, swapChain.outputImageViews);
}

uint32_t VulkanRenderer::pickId(VkDescriptorSet& cameraDS, uint32_t pixelX, uint32_t pixelY)
{
	vkDeviceWaitIdle(context.logicalDevice);

	VkCommandBuffer cmdBuffer = commandPool.beginSingleTimeCommands(context);

	ImageUtils::transitionImageLayout(context, commandPool, idPass.texture, VK_FORMAT_R32_UINT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = idPass.textureView;
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color.uint32[0] = 0;

	VkRenderingAttachmentInfo depthAttachment{};
	depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	depthAttachment.imageView = swapChain.depthResolveView;
	depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;   // reuse depth from the last real frame
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	VkRenderingInfo renderInfo{};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.renderArea = { {0, 0}, viewportExtent };
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = &colorAttachment;
	renderInfo.pDepthAttachment = &depthAttachment;

	vkCmdBeginRendering(cmdBuffer, &renderInfo);

	VkViewport viewport{ 0, 0, (float)viewportExtent.width, (float)viewportExtent.height, 0.0f, 1.0f };
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	VkRect2D scissor{ {0,0}, viewportExtent };
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	if (editorState == EditorState::Object)
	{
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, idPass.idObjectPipeline);
		for (uint32_t i = 0; i < renderObjects.size(); i++)
		{
			RenderObject& obj = renderObjects[i];
			VkBuffer vbufs[] = { obj.mesh.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vbufs, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, obj.mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, idPass.idObjectPipelineLayout, 0, 1, &cameraDS, 0, nullptr);

			IDPushConstants pc{ obj.getModelMatrix(), i + 1 }; // +1: reserve 0
			vkCmdPushConstants(cmdBuffer, idPass.idObjectPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

			vkCmdDrawIndexed(cmdBuffer, obj.mesh.indexCount, 1, 0, 0, 0);
		}
	}
	else
	{
		RenderObject& obj = renderObjects[scene.getSelectedObjId()];
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, idPass.idEditPipeline);
		VkBuffer vbufs[] = { obj.mesh.vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vbufs, offsets);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, idPass.idEditPipelineLayout, 0, 1, &cameraDS, 0, nullptr);

		IDPushConstants pc{ obj.getModelMatrix(), 0 };
		vkCmdPushConstants(cmdBuffer, idPass.idEditPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

		vkCmdDraw(cmdBuffer, static_cast<uint32_t>(obj.mesh.evalVertices.size()), 1, 0, 0);
	}

	vkCmdEndRendering(cmdBuffer);

	ImageUtils::transitionImageLayout(context, cmdBuffer, idPass.texture, VK_FORMAT_R32_UINT,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

	//make sure box isnt outside of extent bounds
	int32_t x = std::clamp((int32_t)(pixelX - idPass.pickRadius), 0, (int32_t)(viewportExtent.width - idPass.boxSize));
	int32_t y = std::clamp((int32_t)(pixelY - idPass.pickRadius), 0, (int32_t)(viewportExtent.height - idPass.boxSize));
	

	VkBufferImageCopy region{};
	region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT /*VkImageAspectFlags */, 0 /*mipLevel*/, 0 /*baseArrayLayer*/, 1 /*layerCount*/ };
	region.imageOffset = { x, y, 0 };
	region.imageExtent = { idPass.boxSize, idPass.boxSize, 1 }; //safe bc min is 0
	vkCmdCopyImageToBuffer(cmdBuffer, idPass.texture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, idPass.readbackBuffer, 1, &region);

	commandPool.endSingleTimeCommands(context, cmdBuffer); //vkQueueWaitIdle until buffer copy has finished

	uint32_t id = 0;
	void* mapped;
	vkMapMemory(context.logicalDevice, idPass.readbackBufferMemory, 0, idPass.boxSize * idPass.boxSize * sizeof(uint32_t), 0, &mapped);
	
	uint32_t* ids = reinterpret_cast<uint32_t*>(mapped);

	int32_t localCursorX = (int32_t)pixelX - x;
	int32_t localCursorY = (int32_t)pixelY - y;

	int32_t closestId = INT32_MAX;

	for (int32_t y = 0; y < idPass.boxSize; y++)
	{
		for (int32_t x = 0; x < idPass.boxSize; x++)
		{
			uint32_t candidate = ids[y * idPass.boxSize + x];
			if (candidate == 0) continue;

			int32_t dx = x - localCursorX;
			int32_t dy = y - localCursorY;
			int32_t distSq = dx * dx + dy * dy;

			if (distSq < closestId)
			{
				closestId = distSq;
				id = candidate;
			}
		}
	}

	vkUnmapMemory(context.logicalDevice, idPass.readbackBufferMemory);

	return id;
}

void VulkanRenderer::updateSelection()
{
	uint32_t id = pickId(frameData.cameraDescriptorSets[currentFrame], pick.x, pick.y);
	if (id != 0)
	{
		switch (editorState)
		{
		case EditorState::Object:
			scene.setSelectedObjId(id - 1);
			break;
		case EditorState::Edit:
			selection.clearSelection();
			selection.selectVertex(id);
		}
	}
	else 
	{
		selection.clearSelection();
	}
	pick.wasClicked = false;
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
	if (!pick.wasClicked)
	{
		pick.x = xpos;
		pick.y = ypos;
	}
}

void VulkanRenderer::onMousePressed(int button, int action, int mods)
{
	if (!guiRenderer->isViewportHovered())
	{
		guiRenderer->handleMouseButton(button, action);

		// if the mouse leaves the viewport while the mouse is pressed release the camera
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		{
			camera.mousePressed = false;
		}
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		camera.mousePressed = true;
		pick.wasClicked = true;

		float localX = ((float)pick.x - guiRenderer->viewportScreenPos.x) * inputScale;
		float localY = ((float)pick.y - guiRenderer->viewportScreenPos.y) * inputScale;

		float maxX = (float)viewportExtent.width - 1.0f;
		float maxY = (float)viewportExtent.height - 1.0f;

		pick.x = static_cast<uint32_t>(std::clamp(localX, 0.0f, maxX));
		pick.y = static_cast<uint32_t>(std::clamp(localY, 0.0f, maxY));
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		camera.mousePressed = false;
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

