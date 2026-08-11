#include "Scene.h"

void Scene::cleanup(VulkanContext& context)
{
	for(RenderObject & obj : objList)
	{
		obj.cleanup(context);
	}
}

void Scene::addObj(VulkanContext& context, CommandPool& cmdPool, std::string modelPath, std::string texturePath,
	VkDescriptorPool& pool, VkDescriptorSetLayout& descriptorSetLayout)
{
	RenderObject obj;
	obj.init(context, cmdPool, modelPath, texturePath, pool, descriptorSetLayout);
	obj.name = "Object " + std::to_string(objList.size() + 1);
	objList.push_back(obj);
	selectedObjId = objList.size() - 1;
}

void Scene::deleteObj(VulkanContext& context, uint32_t index)
{
	objList[selectedObjId].cleanup(context);
	objList.erase(objList.begin() + selectedObjId);
	selectedObjId = objList.size() - 1;
}

uint32_t Scene::getSelectedObjId()
{
	return selectedObjId;
}

void Scene::setSelectedObjId(uint32_t value)
{
	selectedObjId = value;
}
