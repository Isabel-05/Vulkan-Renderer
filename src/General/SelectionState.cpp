#include "SelectionState.h"

void Selection::selectVertex(uint32_t id)
{
	selectedVertexIds.insert(id);
}

void Selection::selectObject(uint32_t id)
{
	selectedObjectId = id;
}

void Selection::clearSelection()
{
	selectedVertexIds.clear();
	selectedObjectId = 0;
}

bool Selection::isVertexSelected(uint32_t id)
{
	return selectedVertexIds.find(id) != selectedVertexIds.end();
}
