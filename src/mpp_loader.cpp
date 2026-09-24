#include "mpp_loader.hpp"

namespace mpp
{

MppLoader::MppLoader()
{

}

std::unique_ptr<MppModel> MppLoader::createModelFromFile(const std::string& file)
{
        // LOADING MESHES ===========================================

    //łądowanie modelu :DDDD
    // Mesh data
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, file.c_str()));

    // zmieniamy pobrane dane na taki format jaki chcemy żeby potem załadować to do gpu
    const VkDeviceSize indexCount{shapes[0].mesh.indices.size()};
    std::vector<MppModel::Vertex> vertices{};
    std::vector<uint16_t> indices{};
    //load vertex and index data
    for (auto& index : shapes[0].mesh.indices) {
        MppModel::Vertex v{
            .pos = {attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2]},
            .normal = {attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2]},
            .uv = {attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]}
        };
        vertices.push_back(v);
        indices.push_back(indices.size());
    }
    /*
    The value of the position's and normal's y-axis, and the texture coordinate's 
    v-axis are flipped. This is done to accommodate for Vulkan's coordinate system. 
    Otherwise the model and the texture image would appear upside down.
    */

    return std::make_unique<MppModel>();


    

}
    
} // namespace mpp
