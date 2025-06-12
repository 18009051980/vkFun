#pragma once

#include <Vertex.h>

#include <vector>

using MyVertex = Vertex<static_cast<VertexAttributeBits>(std::to_underlying(VertexAttributeBits::POSITION) | std::to_underlying(VertexAttributeBits::COLOR) | std::to_underlying(VertexAttributeBits::TEXCOORD) | std::to_underlying(VertexAttributeBits::NORMAL))>;

class ObjLoader
{
public:
    void load(const char* filename, std::vector<MyVertex>& vertices, std::vector<uint32_t>& indices);
};