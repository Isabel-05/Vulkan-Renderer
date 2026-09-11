#include "Modifiers.h"
#include <glm/geometric.hpp>

void FlatShadingMdf::evaluate(std::vector<Vertex>& verts, std::vector<uint32_t>& indx)
{
	std::vector<Vertex> inVerts = verts;
	std::vector<uint32_t> inIndx = indx;
	size_t length = indx.size() / 3;
	indx.clear();
	verts.clear();
	
	for (int i = 0; i < length; i++)
	{
		Vertex v1 = inVerts[inIndx[3 * i + 0]];
		Vertex v2 = inVerts[inIndx[3 * i + 1]];
		Vertex v3 = inVerts[inIndx[3 * i + 2]];

		glm::vec3 normal = glm::cross(v3.position - v1.position, v2.position - v1.position);
		normal = glm::normalize(normal);

		v1.normal = normal;
		v2.normal = normal;
		v3.normal = normal;

		verts.push_back(v1);
		verts.push_back(v2);
		verts.push_back(v3);

		indx.push_back(indx.size());
		indx.push_back(indx.size());
		indx.push_back(indx.size());
	}

}

void FlatShadingMdf::apply(std::vector<Vertex>& verts, std::vector<uint32_t>& indx)
{
}
