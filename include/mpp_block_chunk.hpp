#pragma once

#include "mpp_block_model.hpp"
#include "mpp_game_object.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>
#include <string_view>

namespace mpp::Block
{

constexpr uint32_t CHUNK_SIZE{32};
constexpr uint32_t CHUNK_VOLUME{CHUNK_SIZE*CHUNK_SIZE*CHUNK_SIZE};
using BlockType = uint8_t;

enum class BlockID : BlockType
{
    AIR,
    BARRIER,
    UNBREAKABLEIUM, // niezniszczalium
    DIRT,
    STONE,
    SAND,
    GRASS,
    BRICK,
    WOOD,
    LEAF,
    PLANKS
};


constexpr std::string_view toString(BlockID id) noexcept
{
    switch (id)
    {
        case BlockID::AIR:            return "AIR";
        case BlockID::BARRIER:        return "BARRIER";
        case BlockID::UNBREAKABLEIUM: return "UNBREAKABLEIUM";
        case BlockID::DIRT:           return "DIRT";
        case BlockID::STONE:          return "STONE";
        case BlockID::SAND:           return "SAND";
        case BlockID::GRASS:          return "GRASS";
        case BlockID::BRICK:          return "BRICK";
        case BlockID::WOOD:           return "WOOD";
        case BlockID::LEAF:           return "LEAF";
        case BlockID::PLANKS:         return "PLANKS";
    }
    return "UNKNOWN";
}

enum FaceDirection {
    FRONT = 0,  // +Z
    BACK,       // -Z
    LEFT,       // -X
    RIGHT,      // +X
    TOP,        // -Y 
    BOTTOM      // +Y
};

// Wektory przesunięcia do sąsiadów
constexpr glm::ivec3 FACE_OFFSETS[6] = {
    { 0,  0,  1}, // FRONT (+Z)
    { 0,  0, -1}, // BACK (-Z)
    {-1,  0,  0}, // LEFT (-X)
    { 1,  0,  0}, // RIGHT (+X)
    { 0, -1,  0}, // TOP (-Y)
    { 0,  1,  0}  // BOTTOM (+Y)
};

// Wektory normalne
constexpr glm::vec3 FACE_NORMALS[6] = {
    { 0.0f,  0.0f,  1.0f},
    { 0.0f,  0.0f, -1.0f},
    {-1.0f,  0.0f,  0.0f},
    { 1.0f,  0.0f,  0.0f},
    { 0.0f, -1.0f,  0.0f},
    { 0.0f,  1.0f,  0.0f}
};

// 4 wierzchołki tworzące kwadrat (ściankę) dla każdego z 6 kierunków
constexpr glm::vec3 FACE_VERTICES[6][4] = {
    // FRONT (+Z)
    { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} },
    // BACK (-Z)
    { {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0} },
    // LEFT (-X)
    { {0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0} },
    // RIGHT (+X)
    { {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1} },
    // TOP (-Y - góra)
    { {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} },
    // BOTTOM (+Y - dół)
    { {0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0} }
};

constexpr glm::vec2 FACE_UVS[4] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {1.0f, 1.0f},
    {0.0f, 1.0f}
};

struct RaycastHit {
    bool hit{false};
    glm::ivec3 blockPos{};     
    glm::ivec3 previousPos{};  
};


class MppBlockChunk
{
private:
    std::vector<BlockID> data{std::vector<BlockID>(CHUNK_VOLUME, BlockID::AIR)};
    bool dirty{false};

    size_t index(size_t x, size_t y, size_t z)const;
    size_t index(glm::u32vec3 xyz)const;
    void populateData();
    bool isTransparent(int x, int y, int z) const;
    bool isSolid(int x, int y, int z) const;
    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const;
public:
    MppBlockChunk(MppDevice& device);
    ~MppBlockChunk(){};

    MppBlockChunk(const MppBlockChunk&) = delete;
    MppBlockChunk& operator=(const MppBlockChunk&) = delete;


    void calculateMesh();
    glm::vec3 determinePlayerStartingPosition();
    void checkCollision(glm::vec3& playerPosition, glm::vec3& playerVelocity, float dt);
    bool placeBlock(glm::vec3& playerPosition, glm::vec3& playerRotation, BlockID block);
    bool destroyBlock(glm::vec3& playerPosition, glm::vec3& playerRotation);
    

    TransformComponent transform{};
    MppBlockModel model;
    bool isDirty(){return dirty;}
};


} // namespace mpp::Block
