#pragma once

#include "DMesh.h"

class Scene
{
public:

	Scene() = default;
	~Scene() = default;


	void addObj(DMesh& dmesh);
	void removeObj(uint32_t index);

	uint32_t getSelectedObjId();
	void setSelectedObjId(uint32_t value);

	std::vector<DMesh> objList;
	bool isDirty = true;
private:
	uint32_t selectedObjId = 0;
};

