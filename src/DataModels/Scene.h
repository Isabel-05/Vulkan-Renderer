#pragma once

#include "DMesh.h"

#include <memory>

class Scene
{
public:

	Scene() = default;
	~Scene() = default;


	void addObj(DMesh& dmesh);
	void removeObj(uint32_t index);

	uint32_t getSelectedObjId();
	void setSelectedObjId(uint32_t value);
	std::shared_ptr<DMesh> getSelectedObj() { return objList[getSelectedObjId()]; }

	std::vector<std::shared_ptr<DMesh>> objList;
	bool isDirty = true;
private:
	uint32_t selectedObjId = 0;
};

