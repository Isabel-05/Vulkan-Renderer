#include "ShaderResources.h"
#include "MaterialUtils.h"
#include "GpuMeshCache.h"


void ShaderResources::cleanup(VulkanContext& context)
{
	//destroying descriptor pool also implicitly frees the descriptor sets allocated from it, so we dont have to free those individually
	vkDestroyDescriptorPool(context.logicalDevice, descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(context.logicalDevice, cameraDSLayout, nullptr);
	vkDestroyDescriptorSetLayout(context.logicalDevice, outlineDSLayout, nullptr);

	vkDestroyPipeline(context.logicalDevice, BaseShaderPl, nullptr);
	vkDestroyPipelineLayout(context.logicalDevice, BaseShaderLayout, nullptr);

	vkDestroyPipeline(context.logicalDevice, LineShaderPl, nullptr);
	vkDestroyPipelineLayout(context.logicalDevice, LineShaderLayout, nullptr);

	vkDestroyPipeline(context.logicalDevice, PointShaderPl, nullptr);
	vkDestroyPipelineLayout(context.logicalDevice, PointShaderLayout, nullptr);

	vkDestroyPipeline(context.logicalDevice, OutlineShaderPl, nullptr);
	vkDestroyPipelineLayout(context.logicalDevice, OutlineShaderLayout, nullptr);

	vkDestroyPipeline(context.logicalDevice, GridShaderPl, nullptr);
	vkDestroyPipelineLayout(context.logicalDevice, GridShaderLayout, nullptr);
}

void ShaderResources::createDescriptorSetLayouts(VulkanContext& context)
{
	//uniform buffer layout for Model view projection matrices
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding cameraBinding = uboLayoutBinding;
	VkDescriptorSetLayoutCreateInfo cameralayoutInfo{};
	cameralayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	cameralayoutInfo.bindingCount = 1;
	cameralayoutInfo.pBindings = &cameraBinding;

	if (vkCreateDescriptorSetLayout(context.logicalDevice, &cameralayoutInfo, nullptr, &cameraDSLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}

	VkDescriptorSetLayoutBinding idTextureBinding{};
	idTextureBinding.binding = 0;
	idTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	idTextureBinding.descriptorCount = 1;
	idTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	idTextureBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo outlineLayoutInfo{};
	outlineLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	outlineLayoutInfo.bindingCount = 1;
	outlineLayoutInfo.pBindings = &idTextureBinding;

	if (vkCreateDescriptorSetLayout(context.logicalDevice, &outlineLayoutInfo, nullptr, &outlineDSLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create id texture descriptor set layout!");
	}
}

void ShaderResources::createDescriptorPool(VulkanContext& context)
{
	VkDescriptorPoolSize poolSizes[2]{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1000;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1000;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 2;
	poolInfo.pPoolSizes = poolSizes;
	poolInfo.maxSets = 1000;

	if (vkCreateDescriptorPool(context.logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void ShaderResources::createPipelines(VulkanContext& context, const VkFormat& swapchainFormat)
{
	VkBool32 compareOp = VK_TRUE;

	MaterialUtils::createPipeline(
		context,
		std::string(SHADER_DIR) + "BaseVert.spv",
		std::string(SHADER_DIR) + "BaseFrag.spv",
		cameraDSLayout,
		VK_TRUE,
		VK_TRUE,
		compareOp,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_POLYGON_MODE_FILL,
		swapchainFormat,
		64, // glm::mat4
		context.msaaSamples,
		SurfaceVertexAttribs::getBindingDescription(),
		SurfaceVertexAttribs::getAttributeDescriptions(),
		BaseShaderPl,
		BaseShaderLayout
	);

	MaterialUtils::createPipeline(
		context,
		std::string(SHADER_DIR) + "LineVert.spv",
		std::string(SHADER_DIR) + "LineFrag.spv",
		cameraDSLayout,
		VK_TRUE,
		VK_FALSE,
		compareOp,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		VK_POLYGON_MODE_LINE,
		swapchainFormat,
		64,
		context.msaaSamples,
		PointVertexAttribs::getBindingDescription(),
		PointVertexAttribs::getAttributeDescriptions(),
		LineShaderPl,
		LineShaderLayout
	);

	MaterialUtils::createPipeline(
		context,
		std::string(SHADER_DIR) + "PointVert.spv",
		std::string(SHADER_DIR) + "PointFrag.spv",
		cameraDSLayout,
		VK_TRUE,
		VK_FALSE,
		compareOp,
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		VK_POLYGON_MODE_POINT,
		swapchainFormat,
		64,
		context.msaaSamples,
		PointVertexAttribs::getBindingDescription(),
		PointVertexAttribs::getAttributeDescriptions(),
		PointShaderPl,
		PointShaderLayout
	);

	createOutlinePipeline(context, swapchainFormat);
	createGridPipeline(context, swapchainFormat, context.msaaSamples);
}

void ShaderResources::createOutlinePipeline(VulkanContext& context, const VkFormat& swapchainFormat)
{
	auto vertShaderCode = BufferUtils::readFile(std::string(SHADER_DIR) + "OutlineVert.spv");
	auto fragShaderCode = BufferUtils::readFile(std::string(SHADER_DIR) + "OutlineFrag.spv");

	VkShaderModule vertShaderModule = BufferUtils::createShaderModule(context, vertShaderCode);
	VkShaderModule fragShaderModule = BufferUtils::createShaderModule(context, fragShaderCode);

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

	//fullscreen triangle generated in vertex shader
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;

	//alpha-blend the outline ring over whatever is already in the viewport target
	VkPipelineColorBlendAttachmentState blendState{};
	blendState.blendEnable = VK_TRUE;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.colorBlendOp = VK_BLEND_OP_ADD;
	blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &blendState;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(OutlinePushConstants);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &outlineDSLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(context.logicalDevice, &pipelineLayoutInfo, nullptr, &OutlineShaderLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create outline pipeline layout!");
	}

	VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
	pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingInfo.colorAttachmentCount = 1;
	pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &pipelineRenderingInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = OutlineShaderLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(context.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &OutlineShaderPl) != VK_SUCCESS) {
		throw std::runtime_error("failed to create outline pipeline!");
	}

	vkDestroyShaderModule(context.logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(context.logicalDevice, vertShaderModule, nullptr);
}

void ShaderResources::createGridPipeline(VulkanContext& context, const VkFormat& swapchainFormat, VkSampleCountFlagBits samples)
{
	auto vertShaderCode = BufferUtils::readFile(std::string(SHADER_DIR) + "GridVert.spv");
	auto fragShaderCode = BufferUtils::readFile(std::string(SHADER_DIR) + "GridFrag.spv");

	VkShaderModule vertShaderModule = BufferUtils::createShaderModule(context, vertShaderCode);
	VkShaderModule fragShaderModule = BufferUtils::createShaderModule(context, fragShaderCode);

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

	//fullscreen triangle generated in the vertex shader, no vertex buffer
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = samples;
	multisampling.sampleShadingEnable = VK_FALSE;

	//alpha-blend the grid over whatever is already in the viewport target
	VkPipelineColorBlendAttachmentState blendState{};
	blendState.blendEnable = VK_TRUE;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blendState.colorBlendOp = VK_BLEND_OP_ADD;
	blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &blendState;

	//test and write *real* depth (computed in the shader from the ray/plane hit)
	//so the grid is correctly hidden behind, and correctly hides, scene geometry
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(GridPushConstants);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &cameraDSLayout;          // reuses the camera UBO set
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(context.logicalDevice, &pipelineLayoutInfo, nullptr, &GridShaderLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create grid pipeline layout!");
	}

	VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
	pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingInfo.colorAttachmentCount = 1;
	pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;
	pipelineRenderingInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = &pipelineRenderingInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = GridShaderLayout;
	pipelineInfo.renderPass = VK_NULL_HANDLE;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(context.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &GridShaderPl) != VK_SUCCESS) {
		throw std::runtime_error("failed to create grid pipeline!");
	}

	vkDestroyShaderModule(context.logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(context.logicalDevice, vertShaderModule, nullptr);
}

