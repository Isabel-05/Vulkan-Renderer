#pragma once

#include "RenderObject.h"

class Scene
{
public:

	Scene() = default;
	~Scene() = default;

	void cleanup(VulkanContext& context);

	void addObj(VulkanContext& context, CommandPool& cmdPool, std::string modelPath, std::string texturePath,
		VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout);
	void deleteObj(VulkanContext& context, uint32_t index);

	uint32_t getSelectedObjId();
	void setSelectedObjId(uint32_t value);

	std::vector<RenderObject> objList;

private:
	uint32_t selectedObjId = 0;
};

