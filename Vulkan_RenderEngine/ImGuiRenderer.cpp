#include "ImGuiRenderer.h"
#include "VulkanContext.h"
#include "BufferUtils.h"
#include "Image.h"

#include <iostream>
#include "imgui_internal.h"


ImGuiRenderer::ImGuiRenderer(VulkanContext& vContext, uint32_t maxFramesInFlight) :
	context(&vContext), graphicsQueueFamily(vContext.getQueueFamilyIndices(vContext.physicalDevice).graphicsFamily)
{
	vertexBuffers.resize(maxFramesInFlight);
	indexBuffers.resize(maxFramesInFlight);
	vertexBufferMemories.resize(maxFramesInFlight);
	indexBufferMemories.resize(maxFramesInFlight);
	vertexCounts.resize(maxFramesInFlight, 0);
	indexCounts.resize(maxFramesInFlight, 0);

	for (int i = 0; i < maxFramesInFlight; i++)
	{
		BufferUtils::createBuffer(
			vContext,
			1024 * sizeof(ImDrawVert),
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffers[i],
			vertexBufferMemories[i]);

		BufferUtils::createBuffer(
			vContext,
			1024 * sizeof(ImDrawVert),
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
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
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable multi-viewport support

	// Inform ImGui that we support the new texture update protocol (v1.92+)
	// This enables support for dynamic font textures and multiple texture atlases
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	// Set display size
	io.DisplaySize = ImVec2(width, height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	//Style
	setStyle();
}

void ImGuiRenderer::cleanup()
{

	vkDestroyImage((*context).logicalDevice, fontImage, nullptr);
	vkFreeMemory((*context).logicalDevice, fontImageMemory, nullptr);
	vkDestroyImageView((*context).logicalDevice, fontImageView, nullptr);

	vkDestroySampler((*context).logicalDevice, sampler, nullptr);
	vkDestroyDescriptorPool((*context).logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout((*context).logicalDevice, descriptorSetLayout, nullptr);
	vkDestroyPipelineCache((*context).logicalDevice, pipelineCache, nullptr);
	vkDestroyPipelineLayout((*context).logicalDevice, pipelineLayout, nullptr);
	vkDestroyPipeline((*context).logicalDevice, pipeline, nullptr);
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
	samplerInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;  // Structure type identifier
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
	VkDescriptorPoolSize poolSize{ VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50 };

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;  // Structure type identifier
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
	layoutInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;  // Structure type identifier
	layoutInfo.bindingCount = 1;                                               // Number of bindings in layout
	layoutInfo.pBindings = &binding;                                           // Binding configuration array

	vkCreateDescriptorSetLayout((*context).logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);       // Create layout object

	// Allocate descriptor set from pool using the defined layout
	// This creates the actual binding that connects GPU resources to shaders
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;  // Structure type identifier
	allocInfo.descriptorPool = descriptorPool;                                // Source pool for allocation
	allocInfo.descriptorSetCount = 1;                                          // Number of sets to allocate
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };                // Layout template array
	allocInfo.pSetLayouts = layouts;                                           // Layout configuration

	vkAllocateDescriptorSets((*context).logicalDevice, &allocInfo, &descriptorSet); // Allocate and store set


	initTexture();
	// Update descriptor set with actual font texture and sampler resources
	// This final step connects the physical GPU resources to the shader binding points
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;           // Expected image layout
	imageInfo.imageView = fontImageView;                           // Font texture view
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet writeSet{};
	writeSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;  // Structure type identifier
	writeSet.dstSet = descriptorSet;                                          // Target descriptor set
	writeSet.descriptorCount = 1;                                              // Number of resources to bind
	writeSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;       // Resource type
	writeSet.pImageInfo = &imageInfo;                                          // Image resource information
	writeSet.dstBinding = 0;                                                   // Binding point in shader

	vkUpdateDescriptorSets((*context).logicalDevice, 1, &writeSet, 0, nullptr);                   // Execute the binding update


	////////////////////////////////////////////////
	// Pipeline creation


	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheInfo{};
	pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	vkCreatePipelineCache((*context).logicalDevice, &pipelineCacheInfo, nullptr, &pipelineCache);

	// Create pipeline layout
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstBlock);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	VkDescriptorSetLayout setLayouts[] = { descriptorSetLayout };
	pipelineLayoutInfo.pSetLayouts = setLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	vkCreatePipelineLayout((*context).logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout);

	auto vertShaderCode = BufferUtils::readFile("C:/Users/Administrator/Documents/Projects/Graphics Programming/repos/Vulkan_RenderEngine/shaders/imgui_vert.spv");
	auto fragShaderCode = BufferUtils::readFile("C:/Users/Administrator/Documents/Projects/Graphics Programming/repos/Vulkan_RenderEngine/shaders/imgui_frag.spv");

	VkShaderModule vertShaderModule = BufferUtils::createShaderModule((*context), vertShaderCode);
	VkShaderModule fragShaderModule = BufferUtils::createShaderModule((*context), fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkVertexInputBindingDescription bindingDesc{};
	bindingDesc.binding = 0;
	bindingDesc.stride = sizeof(ImDrawVert);
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attrDescs[3]{};
	attrDescs[0].location = 0;
	attrDescs[0].binding = 0;
	attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescs[0].offset = offsetof(ImDrawVert, pos);

	attrDescs[1].location = 1;
	attrDescs[1].binding = 0;
	attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescs[1].offset = offsetof(ImDrawVert, uv);

	attrDescs[2].location = 2;
	attrDescs[2].binding = 0;
	attrDescs[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attrDescs[2].offset = offsetof(ImDrawVert, col);

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &bindingDesc;
	vertexInput.vertexAttributeDescriptionCount = 3;
	vertexInput.pVertexAttributeDescriptions = attrDescs;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewport state  both are dynamic so counts = 1, pointers = null
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	//anti aliasing
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

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

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &blendState;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

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

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Dynamic rendering chained via pNext, no VkRenderPass needed
	VkPipelineRenderingCreateInfo dynRenderInfo{};
	dynRenderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	dynRenderInfo.colorAttachmentCount = 1;
	dynRenderInfo.pColorAttachmentFormats = &colorFormat;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &dynRenderInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines((*context).logicalDevice, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule((*context).logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule((*context).logicalDevice, fragShaderModule, nullptr);
}

void ImGuiRenderer::initTexture()
{
	ImageUtils::createImage(
		(*context), 1280, 720, VkFormat::VK_FORMAT_B8G8R8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		fontImage, fontImageMemory);

	ImageUtils::createImageView(
		(*context), fontImage, VkFormat::VK_FORMAT_B8G8R8A8_UNORM, VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, fontImageView, 1);
}

void ImGuiRenderer::setStyle()
{
	ImVec4 black = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	ImVec4 lightPurple = ImVec4(0.42f, 0.42f, 0.9f, 1.0f);
	ImVec4 darkPurple = ImVec4(0.3f, 0.2f, 0.57f, 1.0f);


	ImGui::StyleColorsClassic();
	ImGuiStyle& style = ImGui::GetStyle();

	style.FontSizeBase = 16.0f;
	style.Colors[ImGuiCol_WindowBg] = black;
	style.Colors[ImGuiCol_TitleBg] = black;
	style.Colors[ImGuiCol_MenuBarBg] = black;
	style.Colors[ImGuiCol_Header] = lightPurple;
	style.Colors[ImGuiCol_HeaderHovered] = darkPurple;

}

void ImGuiRenderer::updateTexture(CommandPool& cmdPool, ImTextureData* tex)
{
	if (tex->Status == ImTextureStatus_WantCreate || tex->Status == ImTextureStatus_WantUpdates) {
		int texWidth = tex->Width;
		int texHeight = tex->Height;
		unsigned char* fontData = (unsigned char*)tex->Pixels;

		if (!fontData) return;

		VkDeviceSize uploadSize = texWidth * texHeight * tex->BytesPerPixel;
		VkFormat format = (tex->BytesPerPixel == 4) ? VkFormat::VK_FORMAT_B8G8R8A8_UNORM : VkFormat::VK_FORMAT_R8_UNORM;

		if (tex->Status == ImTextureStatus_WantCreate) {

			vkDeviceWaitIdle((*context).logicalDevice);

			vkDestroyImage((*context).logicalDevice, fontImage, nullptr);
			vkFreeMemory((*context).logicalDevice, fontImageMemory, nullptr);
			vkDestroyImageView((*context).logicalDevice, fontImageView, nullptr);

			// Create optimized GPU image for texture storage
			VkExtent3D extent{ static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };
			ImageUtils::createImage(
				(*context), extent.width, extent.height, format,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				fontImage, fontImageMemory);

			ImageUtils::createImageView(
				(*context), fontImage, format, VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT, fontImageView, 1);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = fontImageView;
			imageInfo.sampler = sampler;

			VkWriteDescriptorSet writeSet{};
			writeSet.sType = VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSet.dstSet = descriptorSet;
			writeSet.dstBinding = 0;
			writeSet.dstArrayElement = 0;
			writeSet.descriptorCount = 1;
			writeSet.descriptorType = VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeSet.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets((*context).logicalDevice, 1, &writeSet, 0, nullptr);
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
		ImageUtils::transitionImageLayout((*context), cmdPool, fontImage, format, VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
		BufferUtils::copyBufferToImage((*context), cmdPool, stagingBuffer, fontImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		ImageUtils::transitionImageLayout((*context), cmdPool, fontImage, format, VkImageLayout::VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VkImageLayout::VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

		vkDestroyBuffer((*context).logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory((*context).logicalDevice, stagingBufferMemory, nullptr);

		// Store descriptor set handle as the ImTextureID
		// In this implementation, we use a single descriptor set for the font atlas
		tex->SetTexID((ImTextureID)(intptr_t)(VkDescriptorSet)descriptorSet);
		tex->SetStatus(ImTextureStatus_OK);
	}
	if (tex->Status == ImTextureStatus_WantDestroy) {
		// Handle texture deletion if needed
		vkDestroyImage((*context).logicalDevice, fontImage, nullptr);
		vkFreeMemory((*context).logicalDevice, fontImageMemory, nullptr);
		vkDestroyImageView((*context).logicalDevice, fontImageView, nullptr);
		tex->SetStatus(ImTextureStatus_Destroyed);
	}
}

void ImGuiRenderer::newFrame()
{
	ImGui::NewFrame();

	ImGuiID dockspace_id = ImGui::GetID("My Dockspace");
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Submit dockspace
	ImGui::DockSpaceOverViewport(dockspace_id, viewport, ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::Begin("Vulkan ImGui Demo");
	ImGui::ShowStyleEditor();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open File")) {}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {} // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "Ctrl+X")) {}
			if (ImGui::MenuItem("Copy", "Ctrl+C")) {}
			if (ImGui::MenuItem("Paste", "Ctrl+V")) {}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	ImGui::ShowDemoWindow();

	ImGui::End();

	// End the frame
	ImGui::EndFrame();

	// Render to generate draw data
	ImGui::Render();

	ImDrawData* drawData = ImGui::GetDrawData();
	drawData->CmdListsCount = drawData->CmdLists.Size;

	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
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
	if (drawData->TotalVtxCount > vertexCounts[currentFrame]) {

		// Recreate vertex buffer with new size
		vkDestroyBuffer((*context).logicalDevice, vertexBuffers[currentFrame], nullptr);
		vkFreeMemory((*context).logicalDevice, vertexBufferMemories[currentFrame], nullptr);

		BufferUtils::createBuffer(
			(*context),
			vertexBufferSize,
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffers[currentFrame],
			vertexBufferMemories[currentFrame]);

		vertexCounts[currentFrame] = drawData->TotalVtxCount;
	}

	if (drawData->TotalIdxCount > indexCounts[currentFrame]) {
		// Recreate index buffer with new size

		vkDestroyBuffer((*context).logicalDevice, indexBuffers[currentFrame], nullptr);
		vkFreeMemory((*context).logicalDevice, indexBufferMemories[currentFrame], nullptr);

		BufferUtils::createBuffer(
			(*context),
			indexBufferSize,
			VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBuffers[currentFrame],
			indexBufferMemories[currentFrame]);

		indexCounts[currentFrame] = drawData->TotalIdxCount;
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

void ImGuiRenderer::recordCmdBuffer(uint32_t currentFrame, VkCommandBuffer& commandBuffer, CommandPool& cmdPool, VkImageView& imageView)
{
	ImDrawData* drawData = ImGui::GetDrawData();
	if (!drawData || drawData->CmdListsCount == 0) {
		return;
	}

	// Process dynamic texture updates (RendererHasTextures protocol)
	if (drawData->Textures) {
		for (int n = 0; n < drawData->Textures->Size; n++) {
			ImTextureData* tex = (*drawData->Textures)[n];
			if (tex->Status != ImTextureStatus_OK) {
				updateTexture(cmdPool, tex);
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

	VkBuffer vBuffers[] = { vertexBuffers[currentFrame] };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffers[currentFrame], 0, VK_INDEX_TYPE_UINT16);

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

void ImGuiRenderer::handleKey(int key, int scancode, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();

	// Map the platform-specific key action to a boolean state
	// In GLFW: GLFW_RELEASE = 0, GLFW_PRESS = 1, GLFW_REPEAT = 2
	bool pressed = (action != 0);

	// Modern ImGui (v1.87+) uses AddKeyEvent to queue input events.
	// This handles key states, modifiers, and repeat logic internally.
	// Most backends can cast native key codes directly to ImGuiKey.
	io.AddKeyEvent((ImGuiKey)key, pressed);
}

void ImGuiRenderer::handleMousePos(float x, float y)
{
	ImGuiIO& io = ImGui::GetIO();
	// Modern event API for mouse position
	io.AddMousePosEvent(x, y);
}

void ImGuiRenderer::handleMouseButton(int button, bool pressed)
{
	ImGuiIO& io = ImGui::GetIO();
	// Modern event API for mouse buttons (0: Left, 1: Right, 2: Middle)
	io.AddMouseButtonEvent(button, pressed);
}

bool ImGuiRenderer::getWantKeyCapture()
{
	return ImGui::GetIO().WantCaptureKeyboard;
}

void ImGuiRenderer::charPressed(uint32_t key)
{
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter(key);
}
