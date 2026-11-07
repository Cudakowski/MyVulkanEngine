#include "mpp_model.hpp"

#include "mpp_utils.hpp"

// #ifndef ENGINE_DIR
// #define ENGINE_DIR "../"
// #endif

namespace std
{
template<>
struct hash<mpp::MppModel::Vertex> {
    size_t operator()(mpp::MppModel::Vertex const &vertex) const{
        size_t seed = 0;
        mpp::hashCombine(
            seed, 
            vertex.pos.x, vertex.pos.y, vertex.pos.z, 
            vertex.normal.x, vertex.normal.y, vertex.normal.z, 
            vertex.uv.x, vertex.uv.y
        );
        return seed;
    }
};
} // namespace std

namespace mpp
{

std::unique_ptr<MppModel> MppModel::createModelFromFile(MppDevice& device, const std::string &filepath)
{
    Builder builder{};
    //builder.loadModel(ENGINE_DIR + filepath);
    builder.loadModel(filepath);
    return std::make_unique<MppModel>(device,builder);
}

void MppModel::Builder::loadModel(const std::string &filepath)
{
    // LOADING MESHES ===========================================

    //łądowanie modelu :DDDD
    // Mesh data
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn{};
    std::string err{};

    if(! tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    vertices.clear();
    indices.clear();
    std::unordered_map<Vertex, uint16_t> uniqueVertices{};

    
    // zmieniamy pobrane dane na taki format jaki chcemy żeby potem załadować to do gpu
    //load vertex and index data
    for (const auto &shape : shapes){
        for (const auto &index : shape.mesh.indices){
            Vertex vertex{};

            if ( index.vertex_index >= 0){//wymagane w .obj, ale i tak sprawdzam
                vertex.pos = {
                    attrib.vertices[3*index.vertex_index + 0],
                   -attrib.vertices[3*index.vertex_index + 1],
                    attrib.vertices[3*index.vertex_index + 2]
                };
            }

            if ( index.normal_index >= 0){//opcjonalne w .obj
                vertex.normal = {
                    attrib.normals[3*index.normal_index + 0],
                   -attrib.normals[3*index.normal_index + 1],
                    attrib.normals[3*index.normal_index + 2]
                };
            } 
            else 
            {   //default
                vertex.normal = {0.0f, -1.0f, 0.0f};
            }

            if ( index.texcoord_index >= 0){//opcjonalne w .obj
                vertex.uv = {
                          attrib.texcoords[2*index.texcoord_index + 0],
                    1.0 - attrib.texcoords[2*index.texcoord_index + 1]
                };
            }
            else 
            {   //default
                vertex.uv = {0.0f, 0.0f};
            }

            auto [it, inserted] = uniqueVertices.try_emplace(vertex, static_cast<uint16_t>(vertices.size()));
            if (inserted) {
                if (vertices.size() >= std::numeric_limits<uint16_t>::max()) {
                    throw std::runtime_error("Model exceeded 65535 unique vertices for uint16_t indices");
                }
                vertices.push_back(vertex);
            }
            indices.push_back(it->second);
        }
    }

    /*
    The value of the position's and normal's y-axis, and the texture coordinate's 
    v-axis are flipped. This is done to accommodate for Vulkan's coordinate system. 
    Otherwise the model and the texture image would appear upside down.
    */
}


MppModel::MppModel(MppDevice &device, const MppModel::Builder &builder): mppDevice{device}, indexCount{static_cast<uint32_t>(builder.indices.size())}
{
    assert(!builder.vertices.empty() && "Vertices cannot be empty!");
    assert(!builder.indices.empty() && "Indices cannot be empty!");

    // tworzenie buffera na vertex i index data
    VkDeviceSize vBufSize = sizeof(MppModel::Vertex) * builder.vertices.size();
    indexDataOffset = (vBufSize + 3) & ~static_cast<VkDeviceSize>(3);
    VkDeviceSize iBufSize{sizeof(uint16_t) * builder.indices.size()};
    
    VkBufferCreateInfo bufferCI{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = indexDataOffset + iBufSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };


    // używamy vma do alokacji pamięci buffera na vertex and index data
    VmaAllocationCreateInfo vBufferAllocCI{
        // flagi do resizable BAR !! :D
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | 
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VmaAllocationInfo vBufferAllocInfo{};
    chk(vmaCreateBuffer(mppDevice.getAllocator(), &bufferCI, &vBufferAllocCI, &vBuffer, &vBufferAllocation, &vBufferAllocInfo));
    
    /* VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
    Without this we'd have to create a so-called "staging" buffer on the host, 
    copy data to that buffer and then submit a buffer copy from staging to the 
    GPU side buffer using a command buffer. That would require a lot more code.
    */

    //kopiowanie danych do vramu
    char* mappedPtr = static_cast<char*>(vBufferAllocInfo.pMappedData);
    memcpy(mappedPtr, builder.vertices.data(), vBufSize);
    memcpy(mappedPtr + indexDataOffset, builder.indices.data(), iBufSize);

    // Bezpieczny flush dla platform bez HOST_COHERENT
    vmaFlushAllocation(mppDevice.getAllocator(), vBufferAllocation, 0, VK_WHOLE_SIZE);// TODO: nie do końca to rozumiem
}

MppModel::~MppModel()
{
    vmaDestroyBuffer(mppDevice.getAllocator(), vBuffer, vBufferAllocation);
}

void MppModel::bind(VkCommandBuffer commandBuffer)
{
    //3D object data
    VkDeviceSize vOffset{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vBuffer, &vOffset);
    vkCmdBindIndexBuffer(commandBuffer, vBuffer, indexDataOffset, VK_INDEX_TYPE_UINT16);
}

void MppModel::draw(VkCommandBuffer commandBuffer)
{
    //DRAW :DDDDD
    vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);//TODO: INSTANCJE OGARNĄĆ
}

} // namespace mpp
