#include "IdRenderpass.h"
#include "MaterialUtils.h"
#include "Image.h"
#include "GpuMeshCache.h"

#include <array>

void IdRenderpass::createResources(VulkanContext& context, VkDescriptorSetLayout cameraDs, VkDescriptorSetLayout outlineDS, VkDescriptorPool pool, VkExtent2D inExtent, uint32_t maxFramesInFlight)
{
    extent = inExtent;

    MaterialUtils::createPipeline(
        context,
        std::string(SHADER_DIR) + "IdObjectVert.spv",
        std::string(SHADER_DIR) + "IdObjectFrag.spv",
        cameraDs,
        VK_FALSE,
        VK_FALSE,
        VK_FALSE,
        VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VkPolygonMode::VK_POLYGON_MODE_FILL,
        VK_FORMAT_R32_UINT,
        sizeof(IdPushConstants),
        VK_SAMPLE_COUNT_1_BIT,
        SurfaceVertexAttribs::getBindingDescription(),
        SurfaceVertexAttribs::getAttributeDescriptions(),
        idObjectPipeline,
        idObjectPipelineLayout
    );

    MaterialUtils::createPipeline(
        context,
        std::string(SHADER_DIR) + "IdEditVert.spv",
        std::string(SHADER_DIR) + "IdEditFrag.spv",
        cameraDs,
        VK_FALSE,
        VK_FALSE,
        VK_TRUE,
        VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VkPolygonMode::VK_POLYGON_MODE_POINT,
        VK_FORMAT_R32_UINT,
        sizeof(IdPushConstants),
        VK_SAMPLE_COUNT_1_BIT,
        PointVertexAttribs::getBindingDescription(),
        PointVertexAttribs::getAttributeDescriptions(),
        idEditPipeline,
        idEditPipelineLayout
    );

	idTextures.resize(maxFramesInFlight);
	idTextureViews.resize(maxFramesInFlight);
	idTextureMemories.resize(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; i++)
    {
        ImageUtils::createImage(
            context,
            inExtent.width,
            inExtent.height,
            VK_FORMAT_R32_UINT,
            VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            idTextures[i],
            idTextureMemories[i]
        );

        ImageUtils::createImageView(
            context,
            idTextures[i],
            VK_FORMAT_R32_UINT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            idTextureViews[i],
            1
        );
    }

    BufferUtils::createBuffer(
        context,
        boxSize * boxSize * sizeof(uint32_t),
        VkBufferUsageFlagBits::VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        readbackBuffer,
        readbackBufferMemory
    );

    //no filtering/linear interpol sampler for uint texture
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(context.logicalDevice, &samplerInfo, nullptr, &outlineSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create id texture sampler!");
    }

    std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, outlineDS);
    VkDescriptorSetAllocateInfo dsAllocInfo{};
    dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    dsAllocInfo.descriptorPool = pool;
    dsAllocInfo.descriptorSetCount = maxFramesInFlight;
    dsAllocInfo.pSetLayouts = layouts.data();

    outlineDescriptorSets.resize(maxFramesInFlight);
    if (vkAllocateDescriptorSets(context.logicalDevice, &dsAllocInfo, outlineDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate id texture descriptor sets!");
    }

	updateDescriptorSets(context);
}

void IdRenderpass::updateDescriptorSets(VulkanContext& context)
{
    for (size_t i = 0; i < outlineDescriptorSets.size(); i++)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = idTextureViews[i];
        imageInfo.sampler = outlineSampler;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = outlineDescriptorSets[i];
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(context.logicalDevice, 1, &writeSet, 0, nullptr);
    }
}


void IdRenderpass::cleanup(VulkanContext& context)
{
	for (int i = 0; i < idTextures.size(); i++)
	{
		vkDestroyImageView(context.logicalDevice, idTextureViews[i], nullptr);
		vkDestroyImage(context.logicalDevice, idTextures[i], nullptr);
		vkFreeMemory(context.logicalDevice, idTextureMemories[i], nullptr);
	}

	vkDestroySampler(context.logicalDevice, outlineSampler, nullptr);

    vkDestroyBuffer(context.logicalDevice, readbackBuffer, nullptr);
    vkFreeMemory(context.logicalDevice, readbackBufferMemory, nullptr);

    vkDestroyPipeline(context.logicalDevice, idObjectPipeline, nullptr);
    vkDestroyPipelineLayout(context.logicalDevice, idObjectPipelineLayout, nullptr);

    vkDestroyPipeline(context.logicalDevice, idEditPipeline, nullptr);
    vkDestroyPipelineLayout(context.logicalDevice, idEditPipelineLayout, nullptr);
}

void IdRenderpass::resize(VulkanContext& context, VkExtent2D inExtent)
{
    extent = inExtent;
    size_t size = idTextures.size();
    
    for (int i = 0; i < size; i++)
    {
        vkDestroyImageView(context.logicalDevice, idTextureViews[i], nullptr);
        vkDestroyImage(context.logicalDevice, idTextures[i], nullptr);
        vkFreeMemory(context.logicalDevice, idTextureMemories[i], nullptr);
    }

    for (uint32_t i = 0; i < size; i++)
    {
        ImageUtils::createImage(
            context,
            inExtent.width,
            inExtent.height,
            VK_FORMAT_R32_UINT,
            VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            idTextures[i],
            idTextureMemories[i]
        );

        ImageUtils::createImageView(
            context,
            idTextures[i],
            VK_FORMAT_R32_UINT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            idTextureViews[i],
            1
        );
    }

	updateDescriptorSets(context);
}