#include "Scene.h"

void Scene::addObj(DMesh& dmesh)
{
	objList.push_back(dmesh);
	isDirty = true;
}

void Scene::removeObj(uint32_t index)
{
	for (int i=0; i<objList.size(); i++)
	{
		if (objList[i].id == index)
		{
			objList.erase(objList.begin() + i - 1);
		}
	}

	isDirty = true;
}

uint32_t Scene::getSelectedObjId()
{
	return selectedObjId;
}

void Scene::setSelectedObjId(uint32_t value)
{
	selectedObjId = value;
}
