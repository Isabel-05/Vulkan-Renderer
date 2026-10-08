#include "IdRenderpass.h"
#include "MaterialUtils.h"
#include "Image.h"
#include "GpuMeshCache.h"

#include <array>

void IdRenderpass::createResources(VulkanContext& context, VkDescriptorSetLayout cameraDs, VkExtent2D inExtent, uint32_t maxFramesInFlight)
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
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
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

}

void IdRenderpass::cleanup(VulkanContext& context)
{
	for (int i = 0; i < idTextures.size(); i++)
	{
		vkDestroyImageView(context.logicalDevice, idTextureViews[i], nullptr);
		vkDestroyImage(context.logicalDevice, idTextures[i], nullptr);
		vkFreeMemory(context.logicalDevice, idTextureMemories[i], nullptr);
	}

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
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
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
}