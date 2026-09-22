#include "IdRenderpass.h"
#include "MaterialUtils.h"
#include "Image.h"

void IdRenderpass::createResources(VulkanContext& context, VkDescriptorSetLayout cameraDs, VkExtent2D inExtent)
{
    extent = inExtent;

    MaterialUtils::createIdPipeline(
        context,
        std::string(SHADER_DIR) + "IdObjectVert.spv",
        std::string(SHADER_DIR) + "IdObjectFrag.spv",
        cameraDs,
        VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        VkPolygonMode::VK_POLYGON_MODE_FILL,
        idObjectPipeline,
        idObjectPipelineLayout
    );

    MaterialUtils::createIdPipeline(
        context,
        std::string(SHADER_DIR) + "IdEditVert.spv",
        std::string(SHADER_DIR) + "IdEditFrag.spv",
        cameraDs,
        VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        VkPolygonMode::VK_POLYGON_MODE_POINT,
        idEditPipeline,
        idEditPipelineLayout
    );

    ImageUtils::createImage(
        context,
        inExtent.width,
        inExtent.height,
        VK_FORMAT_R32_UINT,
        VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VkMemoryPropertyFlagBits::VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        texture,
        textureMemory
    );

    ImageUtils::createImageView(
        context,
        texture,
        VK_FORMAT_R32_UINT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        textureView,
        1
    );

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
    vkDestroyImageView(context.logicalDevice, textureView, nullptr);
    vkDestroyImage(context.logicalDevice, texture, nullptr);
    vkFreeMemory(context.logicalDevice, textureMemory, nullptr);

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

    vkDestroyImageView(context.logicalDevice, textureView, nullptr);
    vkDestroyImage(context.logicalDevice, texture, nullptr);
    vkFreeMemory(context.logicalDevice, textureMemory, nullptr);

    ImageUtils::createImage(context, extent.width, extent.height, VK_FORMAT_R32_UINT,
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture, textureMemory);

    ImageUtils::createImageView(context, texture, VK_FORMAT_R32_UINT, VK_IMAGE_ASPECT_COLOR_BIT, textureView, 1);
}