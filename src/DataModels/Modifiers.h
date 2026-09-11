#pragma once
#include "Vertex.h"
#include <vector>

class MeshModifier
{
public:
    virtual void evaluate(std::vector<Vertex>& verts, std::vector<uint32_t>& indx) = 0;

    virtual void apply(std::vector<Vertex> &verts, std::vector<uint32_t> &indx) = 0;

};

class FlatShadingMdf : public MeshModifier
{
public:
    void evaluate(std::vector<Vertex>& verts, std::vector<uint32_t>& indx) override;

    void apply(std::vector<Vertex>& verts, std::vector<uint32_t>& indx) override;
};