#pragma once
#include <unordered_set>

class Selection 
{
public:
	void selectVertex(uint32_t id);
	void selectObject(uint32_t id);
	void clearSelection();
	bool isVertexSelected(uint32_t id);
	bool isObjectSelected(uint32_t id) { return selectedObjectId == id; }

private:
	std::unordered_set<uint32_t> selectedVertexIds;
	uint32_t selectedObjectId = 0;
};