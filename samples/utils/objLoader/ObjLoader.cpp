#include "ObjLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>
#include <unordered_map>

using namespace std;

template<VertexAttributeBits bits>
    struct V : public
    Attribute_Position<has_attribute<bits, VertexAttributeBits::POSITION>(), glm::vec3>,
    Attribute_Color<has_attribute<bits, VertexAttributeBits::COLOR>(), glm::vec3>,
    Attribute_TexCoord<has_attribute<bits, VertexAttributeBits::TEXCOORD>(), glm::vec2>,
    Attribute_Normal<has_attribute<bits, VertexAttributeBits::NORMAL>(), glm::vec3>
    {};

void faaaa()
{
    Attribute_Position<true, glm::vec3> te;
    te.position;

    
    using TV = V<static_cast<VertexAttributeBits>(std::to_underlying(VertexAttributeBits::POSITION) | std::to_underlying(VertexAttributeBits::COLOR) | std::to_underlying(VertexAttributeBits::TEXCOORD) | std::to_underlying(VertexAttributeBits::NORMAL))>;

    TV t;
    t.position;
    t.color;
    t.texCoord;
    t.normal;

}


//加载obj文件
void ObjLoader::load(const char* filename, std::vector<MyVertex>& vertices, std::vector<uint32_t>& indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warning, err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &err, filename))
    {
        throw std::runtime_error(warning + err);
    }
    std::unordered_map<MyVertex, uint32_t> uniqueVertices{};
    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            MyVertex vertex{};
            vertex.position = glm::vec3{
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };
            vertex.texCoord = glm::vec2{
                attrib.texcoords[2 * index.texcoord_index + 0],
                attrib.texcoords[2 * index.texcoord_index + 1],
            };
            vertex.color = glm::vec3{0.0f, attrib.texcoords[2 * index.texcoord_index + 0], attrib.texcoords[2 * index.texcoord_index + 1]};
            vertex.normal = glm::vec3{
                attrib.normals[2 * index.normal_index + 0],
                attrib.normals[2 * index.normal_index + 1],
                attrib.normals[2 * index.normal_index + 2],
            };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
    
            indices.push_back(uniqueVertices[vertex]);
        }
    }
}