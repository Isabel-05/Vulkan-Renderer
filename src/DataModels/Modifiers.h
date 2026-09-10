#pragma once
#include "Vertex.h"
#include <vector>

class MeshModifier
{
public:
    virtual void evaluate(std::vector<Vertex>& verts, std::vector<uint32_t>& indx);

    virtual void apply(std::vector<Vertex> &verts, std::vector<uint32_t> &indx);

};

class FlatShadingMdf : MeshModifier
{

};