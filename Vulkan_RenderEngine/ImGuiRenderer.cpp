#include "ImGuiRenderer.h"
#include "BufferUtils.h"
#include "Image.h"


ImGuiRenderer::ImGuiRenderer(VulkanRenderer& renderer):
	context(&renderer.context), graphicsQueueFamily(renderer.context.getQueueFamilyIndices(renderer.context.physicalDevice).graphicsFamily)
{
	vertexBuffers.resize(renderer.frameData.maxFramesInFlight);
	indexBuffers.resize(renderer.frameData.maxFramesInFlight);
	vertexBufferMemories.resize(renderer.frameData.maxFramesInFlight);
	indexBufferMemories.resize(renderer.frameData.maxFramesInFlight);

	for (int i = 0; i < renderer.frameData.maxFramesInFlight; i++)
	{
		BufferUtils::createBuffer(
			renderer.context,
			1024 * sizeof(ImDrawVert),
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffers[i],
			vertexBufferMemories[i]);

		BufferUtils::createBuffer(
			renderer.context,
			1024 * sizeof(ImDrawVert),
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBuffers[i],
			indexBufferMemories[i]);
	}

	renderingInfo.colorAttachmentCount = 1;
	VkFormat formats[] = { colorFormat };
	renderingInfo.pColorAttachmentFormats = &colorFormat;
}

ImGuiRenderer::~ImGuiRenderer()
{
}

void ImGuiRenderer::init(float width, float height)
{
	// Initialize ImGui (*context)
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Configure ImGui
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls

	// Inform ImGui that we support the new texture update protocol (v1.92+)
	// This enables support for dynamic font textures and multiple texture atlases
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	// Set display size
	io.DisplaySize = ImVec2(width, height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	// Set up style
	vulkanStyle = ImGui::GetStyle();
	vulkanStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	vulkanStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	vulkanStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	vulkanStyle.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	// Apply default style
	setStyle(0);
}

void ImGuiRenderer::cleanup()
{
	vkDestroySampler((*context).logicalDevice, sampler, nullptr);
	vkDestroyDescriptorPool((*context).logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout((*context).logicalDevice, descriptorSetLayout, nullptr);
	vkDestroyPipelineCache((*context).logicalDevice, pipelineCache, nullptr);
	vkDestroyPipelineLayout((*context).logicalDevice, pipelineLayout, nullptr);
	//vkDestroyPipeline((*context).logicalDevice, pipeline, nullptr);
	for (size_t i = 0; i < vertexBuffers.size(); i++) {
		vkDestroyBuffer((*context).logicalDevice, vertexBuffers[i], nullptr);
		vkFreeMemory((*context).logicalDevice, vertexBufferMemories[i], nullptr);
	}
	for (size_t i = 0; i < indexBuffers.size(); i++) {
		vkDestroyBuffer((*context).logicalDevice, indexBuffers[i], nullptr);
		vkFreeMemory((*context).logicalDevice, indexBufferMemories[i], nullptr);
	}
}

void ImGuiRenderer::initResources()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = VkFilter::VK_FILTER_LINEAR;                    // Smooth scaling when magnified
	samplerInfo.minFilter = VkFilter::VK_FILTER_LINEAR;                    // Smooth scaling when minified
	samplerInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;        // Smooth transitions between mip levels
	samplerInfo.addressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  // Prevent texture wrapping
	samplerInfo.addressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  // Clean edge handling
	samplerInfo.addressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;  // 3D consistency
	samplerInfo.borderColor = VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;   // White border for clamped areas

	vkCreateSampler((*context).logicalDevice, &samplerInfo, nullptr, &sampler);                   // Create the GPU sampler object

	// Create descriptor pool for shader resource binding
	// Descriptors provide the interface between shaders and GPU resources
	VkDescriptorPoolSize poolSize{ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.flags = VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;     // Allow individual descriptor set freeing
	poolInfo.maxSets = 2;                                                      // Maximum number of descriptor sets
	poolInfo.poolSizeCount = 1;                                                // Number of pool size specifications
	poolInfo.pPoolSizes = &poolSize;                                           // Pool size configuration

	vkCreateDescriptorPool((*context).logicalDevice, &poolInfo, nullptr, &descriptorPool);                   // Create descriptor pool

	// Create descriptor set layout defining shader resource interface
	// This layout must match the binding declarations in the ImGui shaders
	VkDescriptorSetLayoutBinding binding{};
	binding.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;        // Combined texture and sampler
	binding.descriptorCount = 1;                                               // Single texture binding
	binding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;                   // Used in fragment shader
	binding.binding = 0;                                                       // Shader binding point 0

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.bindingCount = 1;                                               // Number of bindings in layout
	layoutInfo.pBindings = &binding;                                           // Binding configuration array

	vkCreateDescriptorSetLayout((*context).logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);       // Create layout object

	// Allocate descriptor set from pool using the defined layout
	// This creates the actual binding that connects GPU resources to shaders
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.descriptorPool = descriptorPool;                                // Source pool for allocation
	allocInfo.descriptorSetCount = 1;                                          // Number of sets to allocate
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };                // Layout template array
	allocInfo.pSetLayouts = layouts;                                           // Layout configuration

	vkAllocateDescriptorSets((*context).logicalDevice, &allocInfo, &descriptorSet); // Allocate and store set

	// Update descriptor set with actual font texture and sampler resources
	// This final step connects the physical GPU resources to the shader binding points
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;           // Expected image layout
	imageInfo.imageView = fontImageView;                           // Font texture view
	imageInfo.sampler = sampler;                                              // Texture sampler

	VkWriteDescriptorSet writeSet{};
	writeSet.dstSet = descriptorSet;                                          // Target descriptor set
	writeSet.descriptorCount = 1;                                              // Number of resources to bind
	writeSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;       // Resource type
	writeSet.pImageInfo = &imageInfo;                                          // Image resource information
	writeSet.dstBinding = 0;                                                   // Binding point in shader

	vkUpdateDescriptorSets((*context).logicalDevice, 1, &writeSet, 0, nullptr);                   // Execute the binding update

	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheInfo{};
	vkCreatePipelineCache((*context).logicalDevice, &pipelineCacheInfo, nullptr, &pipelineCache);

	// Create pipeline layout
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstBlock);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.setLayoutCount = 1;
	VkDescriptorSetLayout setLayouts[] = { descriptorSetLayout };
	pipelineLayoutInfo.pSetLayouts = setLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	vkCreatePipelineLayout((*context).logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout);

	// Create the graphics pipeline with dynamic rendering
	// ... (shader loading, pipeline state setup, etc.)

	// For brevity, we're omitting the full pipeline creation code here
	// In a real implementation, you would:
	// 1. Load the vertex and fragment shaders
	// 2. Set up all the pipeline state (vertex input, input assembly, rasterization, etc.)
	// 3. Include the renderingInfo in the pipeline creation to enable dynamic rendering

	// Alpha blending
	VkPipelineColorBlendAttachmentState blendState{};
	blendState.blendEnable = VK_TRUE;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.colorBlendOp = VK_BLEND_OP_ADD;
	blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	// No depth test
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;

	// No backface culling
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
}

void ImGuiRenderer::setStyle(uint32_t index)
{
	ImGuiStyle& style = ImGui::GetStyle();

	switch (index) {
	case 0:
		// Custom Vulkan style
		style = vulkanStyle;
		break;
	case 1:
		// Classic style
		ImGui::StyleColorsClassic();
		break;
	case 2:
		// Dark style
		ImGui::StyleColorsDark();
		break;
	case 3:
		// Light style
		ImGui::StyleColorsLight();
		break;
	}
}

void ImGuiRenderer::updateTexture(VkCommandBuffer& commandBuffer, ImTextureData* tex)
{
	if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates) {
		int texWidth = tex->Width;
		int texHeight = tex->Height;
		unsigned char* fontData = (unsigned char*)tex->Pixels;

		if (!fontData) return;

		VkDeviceSize uploadSize = texWidth * texHeight * tex->BytesPerPixel;
		VkFormat format = (tex->BytesPerPixel == 4) ? VkFormat::VK_FORMAT_B8G8R8A8_UNORM : VkFormat::VK_FORMAT_R8_UNORM;

		if (tex->Status == ImTextureStatus_WantCreate) {
			// Create optimized GPU image for texture storage
			VkExtent3D extent{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };
			ImageUtils::createImage(
				(*context), extent.width, extent.height, format,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				fontImage, fontImageMemory);

			ImageUtils::createImageView(
				(*context), fontImage, format, VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, fontImageView);
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		BufferUtils::createBuffer(
			(*context),
			uploadSize,
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// Copy data to staging buffer
		void* data;
		vkMapMemory((*context).logicalDevice, stagingBufferMemory, 0, uploadSize, 0, &data);
		memcpy(data, fontData, uploadSize);
		vkUnmapMemory((*context).logicalDevice, stagingBufferMemory);

		// Transition image layout and copy data
		ImageUtils::transitionImageLayout((*context), commandBuffer, fontImage, format, VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		BufferUtils::copyBufferToImage((*context), commandBuffer, stagingBuffer, fontImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		ImageUtils::transitionImageLayout((*context), commandBuffer, fontImage, format, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// Store descriptor set handle as the ImTextureID
		// In this implementation, we use a single descriptor set for the font atlas
		tex->SetTexID((ImTextureID)(intptr_t)(VkDescriptorSet)descriptorSet);
		tex->SetStatus(ImTextureStatus_OK);
	}
}

bool ImGuiRenderer::newFrame()
{
	ImGui::NewFrame();

	// Create your UI elements here
	// For example:
	ImGui::Begin("Vulkan ImGui Demo");
	ImGui::Text("Hello, Vulkan!");
	if (ImGui::Button("Click me!")) {
		// Handle button click
	}
	ImGui::End();

	// End the frame
	ImGui::EndFrame();

	// Render to generate draw data
	ImGui::Render();

	// Check if buffers need updating
	ImDrawData* drawData = ImGui::GetDrawData();
	if (drawData && drawData->CmdListsCount > 0) {
		if (drawData->TotalVtxCount > vertexCount || drawData->TotalIdxCount > indexCount) {
			needsUpdateBuffers = true;
			return true;
		}
	}

	return false;
}

void ImGuiRenderer::updateBuffers(uint32_t currentFrame, uint32_t maxFramesInFlight)
{
	ImDrawData* drawData = ImGui::GetDrawData();
	if (!drawData || drawData->CmdListsCount == 0) {
		return;
	}

	// Calculate required buffer sizes
	VkDeviceSize vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

	// Resize buffers if needed
	if (drawData->TotalVtxCount > vertexCount) {
		// Recreate vertex buffer with new size
		for (int i = 0; i < maxFramesInFlight; i++)
		{
			BufferUtils::createBuffer(
				(*context),
				1024 * sizeof(ImDrawVert),
				VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				vertexBuffers[i],
				vertexBufferMemories[i]);
		}
	}

	if (drawData->TotalIdxCount > indexCount) {
		// Recreate index buffer with new size
		for (int i = 0; i < maxFramesInFlight; i++)
		{
			BufferUtils::createBuffer(
				(*context),
				1024 * sizeof(ImDrawVert),
				VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				indexBuffers[i],
				indexBufferMemories[i]);
		}
	}

	// Upload data to buffers
	ImDrawVert* vtxDst = nullptr;
	ImDrawIdx* idxDst = nullptr;

	vkMapMemory((*context).logicalDevice, vertexBufferMemories[currentFrame], 0, VK_WHOLE_SIZE, 0, (void**)&vtxDst);
	vkMapMemory((*context).logicalDevice, indexBufferMemories[currentFrame], 0, VK_WHOLE_SIZE, 0, (void**)&idxDst);

	for (int n = 0; n < drawData->CmdListsCount; n++) {
		const ImDrawList* cmdList = drawData->CmdLists[n];
		memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmdList->VtxBuffer.Size;
		idxDst += cmdList->IdxBuffer.Size;
	}

	vkUnmapMemory((*context).logicalDevice, vertexBufferMemories[currentFrame]);
	vkUnmapMemory((*context).logicalDevice, indexBufferMemories[currentFrame]);
}

void ImGuiRenderer::drawFrame(VkCommandBuffer& commandBuffer, VkImageView& imageView)
{
	ImDrawData* drawData = ImGui::GetDrawData();
	if (!drawData || drawData->CmdListsCount == 0) {
		return;
	}

	// Process dynamic texture updates (v1.92+ RendererHasTextures protocol)
	if (drawData->Textures) {
		for (int n = 0; n < drawData->Textures->Size; n++) {
			ImTextureData* tex = (*drawData->Textures)[n];
			if (tex->Status != ImTextureStatus_OK) {
				updateTexture(commandBuffer, tex);
			}
		}
	}

	// Begin dynamic rendering
	VkRenderingAttachmentInfo colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	colorAttachment.imageView = imageView;                      // your current frame's image view
	colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;             // LOAD to not clear the 3D scene underneath
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.renderArea.offset = { 0, 0 };
	renderingInfo.renderArea.extent = {
		static_cast<uint32_t>(drawData->DisplaySize.x),
		static_cast<uint32_t>(drawData->DisplaySize.y)
	};
	renderingInfo.layerCount = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;

	vkCmdBeginRendering(commandBuffer, &renderingInfo);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// Configure viewport for UI pixel coordinates
	VkViewport viewport{};
	viewport.width = drawData->DisplaySize.x;
	viewport.height = drawData->DisplaySize.y;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	pushConstBlock.scale = glm::vec2(2.0f / drawData->DisplaySize.x, 2.0f / drawData->DisplaySize.y);
	pushConstBlock.translate = glm::vec2(-1.0f);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

	VkBuffer vBuffers[] = { *vertexBuffers.data() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, *indexBuffers.data(), 0, VK_INDEX_TYPE_UINT16);

	int vertexOffset = 0;
	int indexOffset = 0;

	for (int i = 0; i < drawData->CmdListsCount; i++) {
		const ImDrawList* cmdList = drawData->CmdLists[i];

		for (int j = 0; j < cmdList->CmdBuffer.Size; j++) {
			const ImDrawCmd* pcmd = &cmdList->CmdBuffer[j];

			// Clip per draw call
			VkRect2D scissor{};
			scissor.offset.x = std::max(static_cast<int32_t>(pcmd->ClipRect.x), 0);
			scissor.offset.y = std::max(static_cast<int32_t>(pcmd->ClipRect.y), 0);
			scissor.extent.width = static_cast<uint32_t>(pcmd->ClipRect.z - pcmd->ClipRect.x);
			scissor.extent.height = static_cast<uint32_t>(pcmd->ClipRect.w - pcmd->ClipRect.y);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			// Bind font (and any UI) textures for this draw
			// The TexID now stores the actual descriptor set handle (VkDescriptorSet)
			VkDescriptorSet texHandle = (VkDescriptorSet)pcmd->GetTexID();
			if (texHandle) {
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &texHandle, 0, nullptr);
			}
			else {
				// Fallback to default font if no specific texture is bound
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
			}

			// Issue indexed draw for this UI batch
			vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
			indexOffset += pcmd->ElemCount;
		}

		vertexOffset += cmdList->VtxBuffer.Size;
	}

	vkCmdEndRendering(commandBuffer);
}
