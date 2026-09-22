#include "SelectionState.h"

void Selection::selectVertex(uint32_t id)
{
	selectedVertexIds.insert(id);
}

void Selection::clearSelection()
{
	selectedVertexIds.clear();
}

bool Selection::isVertexSelected(uint32_t id)
{
	return selectedVertexIds.find(id) != selectedVertexIds.end();
}
