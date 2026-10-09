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
		commandPool.create(context);
		swapChain.createSwapchain(context);
		swapChain.createImageViews(context);
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

		idPass.createResources(context, shaderResources.cameraDSLayout, shaderResources.outlineDSLayout,
			shaderResources.descriptorPool, swapChain.extent, frameData.maxFramesInFlight);

		// Object for testing purposes
		DMesh dmesh;
		dmesh.init(1, std::string(ASSET_DIR) + "models/BlenderCube.obj");
		scene.addObj(dmesh);
		scene.setSelectedObjId(0);
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

	guiRenderer->newFrame(commandPool, currentFrame, scene, editorState);

	// VIEWPORT RESIZE HANDLING	
	if (checkViewportResize())
		resizeViewportResources();

	// UPDATE VIEW PROJECTION UBO	
	glm::mat4 viewMatrix = camera.getViewMatrix();
	glm::mat4 projectionMatrix = camera.getProjectionMatrix((float)viewportExtent.width / (float)viewportExtent.height, 0.1f, 20.0f);
	updateUniformBuffer(currentFrame, viewMatrix, projectionMatrix);


	vkResetFences(context.logicalDevice, 1, &frameData.inFlightFences[currentFrame]);

	// COMMAND RECORDING START	
	vkResetCommandBuffer(frameData.commandBuffers[currentFrame], 0);

	// VIEWPORT RENDERING COMMANDS
	recordCommandBuffer(frameData.commandBuffers[currentFrame], imageIndex);

	//ID PASS (for outline and selection)
	//pick.wasClicked = true;
	bool wantOutline = editorState == EditorState::Object && scene.getSelectedObjId() != 0;
	if (pick.wasClicked || wantOutline)
	{
		recordIdPass(frameData.commandBuffers[currentFrame], currentFrame, frameData.cameraDescriptorSets[currentFrame]);
	}
	if (wantOutline) 
	{
		// Transition the ID texture to shader read layout for outline rendering
		if (pick.wasClicked) {
			ImageUtils::transitionImageLayout(context, frameData.commandBuffers[currentFrame], idPass.idTextures[currentFrame], VK_FORMAT_R32_UINT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		}
		else {
			ImageUtils::transitionImageLayout(context, frameData.commandBuffers[currentFrame], idPass.idTextures[currentFrame], VK_FORMAT_R32_UINT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
		}
		recordOutlinePass(frameData.commandBuffers[currentFrame], currentFrame);
	}

	//FINISH VIEWPORT RENDERING
	toShaderRead(frameData.commandBuffers[currentFrame], swapChain.outputImages[currentFrame]);

	// IMGUI RENDERING COMMANDS
	guiRenderer->updateBuffers(currentFrame, frameData.maxFramesInFlight);
	ImageUtils::transitionImageLayout(context, frameData.commandBuffers[currentFrame], swapChain.images[imageIndex], swapChain.imageFormat,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	guiRenderer->recordCmdBuffer(currentFrame, frameData.commandBuffers[currentFrame], commandPool, swapChain.imageViews[imageIndex]);

	// COMMAND RECORDING END
	ImageUtils::transitionImageLayout(context, frameData.commandBuffers[currentFrame], swapChain.images[imageIndex], swapChain.imageFormat,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1);

	if (vkEndCommandBuffer(frameData.commandBuffers[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	// SUBMIT START
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

	// PRESENT START	
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.handle };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

	// RESIZE HANDLING
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		resizeViewportResources();
		swapChain.recreateSwapChain(context, commandPool, frameData.maxFramesInFlight);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(swapChain.extent.width), static_cast<float>(swapChain.extent.height));
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	// SELECTION HANDLING
	if (pick.wasClicked)
	{
		vkWaitForFences(context.logicalDevice, 1, &frameData.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t id = pickId(currentFrame, pick.x, pick.y);
		updateSelection(id);
		pick.wasClicked = false;
	}

	currentFrame = (currentFrame + 1) % frameData.maxFramesInFlight;
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	toColorAt(commandBuffer, swapChain.outputImages[currentFrame]);

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

	if (scene.getSelectedObj())
	{
		if (moveFlag && editorState == EditorState::Edit && !scene.getSelectedObj()->getSelectedVertIds().empty())
		{
			uint32_t id = scene.getSelectedObj()->getSelectedVertIds()[0];
			scene.getSelectedObj()->positions[id].y += (ct - pick.y)*0.005;
			scene.getSelectedObj()->markPositionsDirty();
			ct = pick.y;
		}
	}
	
	for (auto& RO : renderObjects)
	{
		RO.sync(context, commandPool, editorState==EditorState::Edit);
		RO.drawSurface(context, commandPool, commandBuffer, frameData.cameraDescriptorSets[currentFrame]);
		
		if (editorState == EditorState::Edit) //todo: and object is selected
		{
			RO.drawEdges(context, commandPool, commandBuffer, frameData.cameraDescriptorSets[currentFrame]);
			RO.drawPoints(context, commandPool, commandBuffer, frameData.cameraDescriptorSets[currentFrame]);
		}
	}

	vkCmdEndRendering(commandBuffer);
}

void VulkanRenderer::recordIdPass(VkCommandBuffer& cmdBuffer, uint32_t currentFrame, VkDescriptorSet& cameraDS)
{
	ImageUtils::transitionImageLayout(context, cmdBuffer, idPass.idTextures[currentFrame], VK_FORMAT_R32_UINT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);

	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = idPass.idTextureViews[currentFrame];
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
			VkBuffer vbufs[] = { obj.gpuCache.surfaceVertBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vbufs, offsets);
			vkCmdBindIndexBuffer(cmdBuffer, obj.gpuCache.surfaceIdxBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, idPass.idObjectPipelineLayout, 0, 1, &cameraDS, 0, nullptr);

			IDPushConstants pc{ obj.getModelMatrix(), renderObjects[i].gpuCache.dataMesh->id };
			vkCmdPushConstants(cmdBuffer, idPass.idObjectPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

			vkCmdDrawIndexed(cmdBuffer, obj.gpuCache.surfaceIdxBuffer.count, 1, 0, 0, 0);
		}
	}
	else
	{
		//safe bc in edit mode we always have a selected object
		//checked in drawloop (if scene.selectedObj() != 0)
		RenderObject* obj;
		for (auto& RO : renderObjects)
		{
			RO.gpuCache.dataMesh->id == scene.getSelectedObjId() ? obj = &RO : void();
		}
		if (!obj) return;

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, idPass.idEditPipeline);
		VkBuffer vbufs[] = { obj->gpuCache.pointBuffer.buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vbufs, offsets);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, idPass.idEditPipelineLayout, 0, 1, &cameraDS, 0, nullptr);

		IDPushConstants pc{ obj->getModelMatrix(), 0 };
		vkCmdPushConstants(cmdBuffer, idPass.idEditPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

		vkCmdDraw(cmdBuffer, obj->gpuCache.pointBuffer.count, 1, 0, 0);
	}

	vkCmdEndRendering(cmdBuffer);

	if (pick.wasClicked)
	{
		ImageUtils::transitionImageLayout(context, cmdBuffer, idPass.idTextures[currentFrame], VK_FORMAT_R32_UINT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

		//make sure box isnt outside of extent bounds
		int32_t x = std::clamp((int32_t)(pick.x - idPass.pickRadius), 0, (int32_t)(viewportExtent.width - idPass.boxSize));
		int32_t y = std::clamp((int32_t)(pick.y - idPass.pickRadius), 0, (int32_t)(viewportExtent.height - idPass.boxSize));


		VkBufferImageCopy region{};
		region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT /*VkImageAspectFlags */, 0 /*mipLevel*/, 0 /*baseArrayLayer*/, 1 /*layerCount*/ };
		region.imageOffset = { x, y, 0 };
		region.imageExtent = { idPass.boxSize, idPass.boxSize, 1 }; //safe bc min is 0
		vkCmdCopyImageToBuffer(cmdBuffer, idPass.idTextures[currentFrame], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, idPass.readbackBuffer, 1, &region);
	}
}

void VulkanRenderer::recordOutlinePass(VkCommandBuffer& cmdBuffer, uint32_t currentFrame)
{
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = swapChain.outputImageViews[currentFrame];
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // keep the already-shaded scene, outline just overlays it
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderInfo{};
	renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderInfo.renderArea = { {0, 0}, viewportExtent };
	renderInfo.layerCount = 1;
	renderInfo.colorAttachmentCount = 1;
	renderInfo.pColorAttachments = &colorAttachment;

	vkCmdBeginRendering(cmdBuffer, &renderInfo);

	VkViewport viewport{ 0, 0, (float)viewportExtent.width, (float)viewportExtent.height, 0.0f, 1.0f };
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	VkRect2D scissor{ {0,0}, viewportExtent };
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderResources.OutlineShaderPl);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderResources.OutlineShaderLayout,
		0, 1, &idPass.outlineDescriptorSets[currentFrame], 0, nullptr);

	OutlinePushConstants pc{};
	pc.selectedId = scene.getSelectedObjId();
	pc.thickness = 2;
	pc.texSize = glm::ivec2((int)viewportExtent.width, (int)viewportExtent.height);
	vkCmdPushConstants(cmdBuffer, shaderResources.OutlineShaderLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(OutlinePushConstants), &pc);

	vkCmdDraw(cmdBuffer, 3, 1, 0, 0); // fullscreen triangle, built from gl_VertexIndex in the vertex shader

	vkCmdEndRendering(cmdBuffer);
}


void VulkanRenderer::updateSelection(uint32_t id)
{
	if (id != 0)
	{
		switch (editorState)
		{
		case EditorState::Object:
			scene.setSelectedObjId(id);
			break;
		case EditorState::Edit:
			std::shared_ptr<DMesh> selectedObj = scene.getSelectedObj();
			selectedObj->clearSelection();
			selectedObj->vertSelected[id-1] = 1;
			selectedObj->markSelectionDirty();
		}
	}
	else
	{
		switch (editorState)
		{
		case EditorState::Object:
			scene.setSelectedObjId(0);
			break;
		case EditorState::Edit:
			std::shared_ptr<DMesh> selectedObj = scene.getSelectedObj();
			selectedObj->clearSelection();
			selectedObj->markSelectionDirty();
		}
	}
}

uint32_t VulkanRenderer::pickId(uint32_t currentFrame, uint32_t pixelX, uint32_t pixelY)
{
	int32_t x = std::clamp((int32_t)(pixelX - idPass.pickRadius), 0, (int32_t)(viewportExtent.width - idPass.boxSize));
	int32_t y = std::clamp((int32_t)(pixelY - idPass.pickRadius), 0, (int32_t)(viewportExtent.height - idPass.boxSize));

	void* mapped;
	vkMapMemory(context.logicalDevice, idPass.readbackBufferMemory, 0, idPass.boxSize * idPass.boxSize * sizeof(uint32_t), 0, &mapped);
	
	uint32_t* ids = reinterpret_cast<uint32_t*>(mapped);

	int32_t localCursorX = (int32_t)pixelX - x;
	int32_t localCursorY = (int32_t)pixelY - y;

	uint32_t id = 0;
	int32_t closestId = INT32_MAX;

	//pick closest id to center of picking box (aka where the input was)
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


void VulkanRenderer::toColorAt(VkCommandBuffer& cmdBuffer, VkImage& image)
{
	VkImageMemoryBarrier2 toColorAttachment{};
	toColorAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	toColorAttachment.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	toColorAttachment.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT; // 0 if first use
	toColorAttachment.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	toColorAttachment.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	toColorAttachment.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; // or SHADER_READ_ONLY_OPTIMAL
	toColorAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	toColorAttachment.image = image;
	toColorAttachment.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkDependencyInfo dep{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dep.imageMemoryBarrierCount = 1;
	dep.pImageMemoryBarriers = &toColorAttachment;
	vkCmdPipelineBarrier2(cmdBuffer, &dep);
}

void VulkanRenderer::toShaderRead(VkCommandBuffer& cmdBuffer, VkImage& image)
{
	VkImageMemoryBarrier2 toShaderRead{};
	toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	toShaderRead.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	toShaderRead.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	toShaderRead.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	toShaderRead.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	toShaderRead.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	toShaderRead.image = image;
	toShaderRead.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	VkDependencyInfo depInfo{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	depInfo.imageMemoryBarrierCount = 1;
	depInfo.pImageMemoryBarriers = &toShaderRead;
	vkCmdPipelineBarrier2(cmdBuffer, &depInfo);
}


bool VulkanRenderer::checkViewportResize()
{
	ImVec2 newAvail = guiRenderer->avail;
	uint32_t newH = std::max(1, (int)newAvail.y);
	uint32_t newW = std::max(1, (int)newAvail.x);

	if (newW != viewportExtent.width || newH != viewportExtent.height)
	{
		viewportExtent = { newW, newH };
		return true;
	}
	return false;
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
	for (auto& obj : renderObjects)
	{
		obj.cleanup(context);
	}
	renderObjects.clear();

	for (auto& dmesh : scene.objList)
	{
		RenderObject newRO;
		newRO.init(context, commandPool, dmesh);
		newRO.name = dmesh->id;

		GpuMaterial basemat;
		basemat.pipeline = std::make_shared<VkPipeline>(shaderResources.BaseShaderPl);
		basemat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.BaseShaderLayout);
		newRO.surfaceMaterial = basemat;

		GpuMaterial edgemat;
		edgemat.pipeline = std::make_shared<VkPipeline>(shaderResources.LineShaderPl);
		edgemat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.LineShaderLayout);
		newRO.edgeMaterial = edgemat;

		GpuMaterial pointmat;
		pointmat.pipeline = std::make_shared<VkPipeline>(shaderResources.PointShaderPl);
		pointmat.pipelineLayout = std::make_shared<VkPipelineLayout>(shaderResources.PointShaderLayout);
		newRO.pointMaterial = pointmat;

		renderObjects.push_back(newRO);
	}
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

/////////////
//PUBLIC API

void VulkanRenderer::onResize()
{
	framebufferResized = true;
}

void VulkanRenderer::onKey(int key, int scancode, int action, int mods)
{
	//action = 1 means press 0 means release
	//std::cout << scancode << std::endl;

	//pressed G
	if (scancode == 34 && action == 1)
	{
		moveFlag = true;
		ct = pick.y;
	}
	//pressed tab
	if (scancode == 15 && action == 1)
		switchEditorState();


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
	moveFlag = false;

	//NOT ON VIEWPORT-> PASS INPUT TO GUI
	if (!guiRenderer->isViewportHovered())
	{
		guiRenderer->handleMouseButton(button, action);

		// if the mouse leaves the viewport while the mouse is pressed release the camera
		if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
		{
			camera.mousePressed = false;
		}
		return;
	}

	//PICKING OBJECT/VERTEX
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		pick.wasClicked = true;

		//convert from glfw global input to viewport/image coordinates
		float localX = ((float)pick.x - guiRenderer->viewportScreenPos.x) * inputScale;
		float localY = ((float)pick.y - guiRenderer->viewportScreenPos.y) * inputScale;

		//make sure inputs are in bounds of the viewport
		float maxX = (float)viewportExtent.width - 1.0f;
		float maxY = (float)viewportExtent.height - 1.0f;
		pick.x = static_cast<uint32_t>(std::clamp(localX, 0.0f, maxX));
		pick.y = static_cast<uint32_t>(std::clamp(localY, 0.0f, maxY));
	}

	//MIDDLE MOUSE-> CAMERA
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		camera.mousePressed = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
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

void VulkanRenderer::switchEditorState()
{
	if (editorState == EditorState::Object && scene.getSelectedObj() != 0)
		editorState = EditorState::Edit;
	else if (editorState == EditorState::Edit)
		editorState = EditorState::Object;
}

