#pragma once
#include "Vertex.h"

class MeshModifier
{
public:
    virtual void evaluate(const std::vector<Vertex> &inVerts, const std::vector<uint32_t> &inIndx,
        const std::vector<Vertex> &outVerts, const std::vector<uint32_t> &outIndx);

    virtual void apply(std::vector<Vertex> &verts, std::vector<uint32_t> &indx);

};

class FlatShadingMdf : MeshModifier
{

};