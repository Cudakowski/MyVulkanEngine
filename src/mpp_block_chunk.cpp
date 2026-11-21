#include "mpp_block_chunk.hpp"
#include "Random.h"

#include "mpp_utils.hpp"

namespace mpp::Block
{


// 0 <= x <= 31, 0 <= y <= 31, 0 <= z <= 31 
size_t MppBlockChunk::index(size_t x, size_t y, size_t z) const
{
    return x + CHUNK_SIZE * y + CHUNK_SIZE * CHUNK_SIZE * z;
}

size_t MppBlockChunk::index(glm::u32vec3 xyz)const
{
    return xyz.x + CHUNK_SIZE * xyz.y + CHUNK_SIZE * CHUNK_SIZE * xyz.z;
    
}


bool MppBlockChunk::isTransparent(int x, int y, int z) const
{
    if (x < 0 || x >= static_cast<int>(CHUNK_SIZE) ||
        y < 0 || y >= static_cast<int>(CHUNK_SIZE) ||
        z < 0 || z >= static_cast<int>(CHUNK_SIZE))
    {
        return true; 
    }

    BlockID block = data[index(x, y, z)];
    return block == BlockID::AIR || block == BlockID::BARRIER;
}

bool MppBlockChunk::isSolid(int x, int y, int z) const
{
    if (x < 0 || x >= static_cast<int>(CHUNK_SIZE) ||
        y < 0 || y >= static_cast<int>(CHUNK_SIZE) ||
        z < 0 || z >= static_cast<int>(CHUNK_SIZE))
    {
        return false; 
    }

    BlockID block = data[index(x, y, z)];
    return block != BlockID::AIR;
}


MppBlockChunk::MppBlockChunk(MppDevice& device) : model{device}
{
    populateData();
}

RaycastHit MppBlockChunk::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const
{
    glm::vec3 relOrigin{origin - transform.translation};
    glm::vec3 dir = glm::normalize(direction);

    glm::ivec3 currentVoxel = glm::ivec3(glm::floor(relOrigin));
    glm::ivec3 prevVoxel = currentVoxel;

    glm::ivec3 step{
        (dir.x > 0) ? 1 : ((dir.x < 0) ? -1 : 0),
        (dir.y > 0) ? 1 : ((dir.y < 0) ? -1 : 0),
        (dir.z > 0) ? 1 : ((dir.z < 0) ? -1 : 0)
    };

    glm::vec3 tDelta{
        (step.x != 0) ? std::abs(1.0f / dir.x) : std::numeric_limits<float>::max(),
        (step.y != 0) ? std::abs(1.0f / dir.y) : std::numeric_limits<float>::max(),
        (step.z != 0) ? std::abs(1.0f / dir.z) : std::numeric_limits<float>::max()
    };

    glm::vec3 tMax{
        (step.x > 0) ? (std::floor(relOrigin.x) + 1.0f - relOrigin.x) * tDelta.x : (relOrigin.x - std::floor(relOrigin.x)) * tDelta.x,
        (step.y > 0) ? (std::floor(relOrigin.y) + 1.0f - relOrigin.y) * tDelta.y : (relOrigin.y - std::floor(relOrigin.y)) * tDelta.y,
        (step.z > 0) ? (std::floor(relOrigin.z) + 1.0f - relOrigin.z) * tDelta.z : (relOrigin.z - std::floor(relOrigin.z)) * tDelta.z
    };

    float distance = 0.0f;

    while (distance <= maxDistance)
    {
        if (currentVoxel.x >= 0 && currentVoxel.x < static_cast<int>(CHUNK_SIZE) &&
            currentVoxel.y >= 0 && currentVoxel.y < static_cast<int>(CHUNK_SIZE) &&
            currentVoxel.z >= 0 && currentVoxel.z < static_cast<int>(CHUNK_SIZE))
        {
            BlockID id = data[index(currentVoxel.x, currentVoxel.y, currentVoxel.z)];
            if (id != BlockID::AIR)
            {
                return RaycastHit{true, currentVoxel, prevVoxel};
            }
        }

        prevVoxel = currentVoxel;

        if (tMax.x < tMax.y)
        {
            if (tMax.x < tMax.z)
            {
                distance = tMax.x;
                tMax.x += tDelta.x;
                currentVoxel.x += step.x;
            }
            else
            {
                distance = tMax.z;
                tMax.z += tDelta.z;
                currentVoxel.z += step.z;
            }
        }
        else
        {
            if (tMax.y < tMax.z)
            {
                distance = tMax.y;
                tMax.y += tDelta.y;
                currentVoxel.y += step.y;
            }
            else
            {
                distance = tMax.z;
                tMax.z += tDelta.z;
                currentVoxel.z += step.z;
            }
        }
    }

    return RaycastHit{false};
}

bool MppBlockChunk::destroyBlock(glm::vec3& playerPosition, glm::vec3& playerRotation)
{
    constexpr float REACH_DISTANCE = 5.0f;
    glm::vec3 eyePos = playerPosition + glm::vec3{0.0f, -1.8f, 0.0f};
    glm::vec3 lookDir{
        glm::cos(playerRotation.x) * glm::sin(playerRotation.y),
        -glm::sin(playerRotation.x),
        glm::cos(playerRotation.x) * glm::cos(playerRotation.y)
    };

    RaycastHit hit = raycast(eyePos, lookDir, REACH_DISTANCE);
    if (!hit.hit) return false;

    BlockID targetBlock = data[index(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z)];


    if (targetBlock == BlockID::BARRIER || targetBlock == BlockID::UNBREAKABLEIUM)
    {
        return false;
    }

    data[index(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z)] = BlockID::AIR;
    dirty = true;
    return true;
}

bool MppBlockChunk::placeBlock(glm::vec3& playerPosition, glm::vec3& playerRotation, BlockID block)
{
    constexpr float REACH_DISTANCE = 5.0f;
    glm::vec3 eyePos = playerPosition + glm::vec3{0.0f, -1.8f, 0.0f};
    glm::vec3 lookDir{
        glm::cos(playerRotation.x) * glm::sin(playerRotation.y),
        -glm::sin(playerRotation.x),
        glm::cos(playerRotation.x) * glm::cos(playerRotation.y)
    };

    RaycastHit hit = raycast(eyePos, lookDir, REACH_DISTANCE);
    if (!hit.hit) return false;

    glm::ivec3 target = hit.previousPos;

    if (target.x < 0 || target.x >= static_cast<int>(CHUNK_SIZE) ||
        target.y < 0 || target.y >= static_cast<int>(CHUNK_SIZE) ||
        target.z < 0 || target.z >= static_cast<int>(CHUNK_SIZE))
    {
        return false;
    }

    if (data[index(target.x, target.y, target.z)] != BlockID::AIR)
    {
        return false;
    }

    // Zabezpieczenie przed postawieniem bloku wewnątrz gracza
    glm::vec3 relPos = playerPosition - transform.translation;
    glm::vec3 pMin = relPos + glm::vec3{-0.4f, -1.9f, -0.4f};
    glm::vec3 pMax = relPos + glm::vec3{ 0.4f,  0.0f,  0.4f};
    glm::vec3 bMin = glm::vec3(target);
    glm::vec3 bMax = bMin + glm::vec3{1.0f, 1.0f, 1.0f};

    bool overlapsPlayer = (pMin.x < bMax.x && pMax.x > bMin.x) &&
                          (pMin.y < bMax.y && pMax.y > bMin.y) &&
                          (pMin.z < bMax.z && pMax.z > bMin.z);
    if (overlapsPlayer)
    {
        return false;
    }

    data[index(target.x, target.y, target.z)] = block;
    dirty = true;
    return true;
}

void MppBlockChunk::checkCollision(glm::vec3& playerPosition, glm::vec3& playerVelocity, float dt)
{
    constexpr float playerWidthL{-0.4f};
    constexpr float playerWidthR{0.4f};
    constexpr float playerLengthF{0.4f};
    constexpr float playerLengthN{-0.4f};
    constexpr float playerHeightU{-1.9f};
    constexpr float playerHeightD{0.0f};
    constexpr float EPS{0.0001f};

    glm::vec3 relPos{playerPosition - transform.translation};
    glm::vec3 displacement{playerVelocity * dt};

    //Y
    if (displacement.y != 0.0f)
    {
        relPos.y += displacement.y;

        glm::ivec3 minB = glm::ivec3(glm::floor(relPos + glm::vec3{playerWidthL + EPS, playerHeightU, playerLengthN + EPS}));
        glm::ivec3 maxB = glm::ivec3(glm::floor(relPos + glm::vec3{playerWidthR - EPS, playerHeightD, playerLengthF - EPS}));

        if (displacement.y < 0.0f) //w górę
        {
            int y = minB.y;
            bool collided = false;
            for (int x = minB.x; x <= maxB.x && !collided; ++x)
            {
                for (int z = minB.z; z <= maxB.z; ++z)
                {
                    if (isSolid(x, y, z))
                    {
                        // Dolna krawędź bloku to (y + 1.0f)
                        relPos.y = static_cast<float>(y + 1) - playerHeightU + EPS;
                        playerVelocity.y = 0.0f;
                        collided = true;
                        break;
                    }
                }
            }
        }
        else // w dół
        {
            int y = maxB.y;
            bool collided = false;
            for (int x = minB.x; x <= maxB.x && !collided; ++x)
            {
                for (int z = minB.z; z <= maxB.z; ++z)
                {
                    if (isSolid(x, y, z))
                    {
                        // Górna krawędź bloku to (y)
                        relPos.y = static_cast<float>(y) - playerHeightD - EPS;
                        playerVelocity.y = 0.0f;
                        collided = true;
                        break;
                    }
                }
            }
        }
    }

    //X
    if (displacement.x != 0.0f)
    {
        relPos.x += displacement.x;

        glm::ivec3 minB = glm::ivec3(glm::floor(relPos + glm::vec3{playerWidthL, playerHeightU + EPS, playerLengthN + EPS}));
        glm::ivec3 maxB = glm::ivec3(glm::floor(relPos + glm::vec3{playerWidthR, playerHeightD - EPS, playerLengthF - EPS}));

        if (displacement.x < 0.0f) // w -X
        {
            int x = minB.x;
            bool collided = false;
            for (int y = minB.y; y <= maxB.y && !collided; ++y)
            {
                for (int z = minB.z; z <= maxB.z; ++z)
                {
                    if (isSolid(x, y, z))
                    {
                        relPos.x = static_cast<float>(x + 1) - playerWidthL + EPS;
                        playerVelocity.x = 0.0f;
                        collided = true;
                        break;
                    }
                }
            }
        }
        else // w +X
        {
            int x = maxB.x;
            bool collided = false;
            for (int y = minB.y; y <= maxB.y && !collided; ++y)
            {
                for (int z = minB.z; z <= maxB.z; ++z)
                {
                    if (isSolid(x, y, z))
                    {
                        relPos.x = static_cast<float>(x) - playerWidthR - EPS;
                        playerVelocity.x = 0.0f;
                        collided = true;
                        break;
                    }
                }
            }
        }
    }

    // Z
    if (displacement.z != 0.0f)
    {
        relPos.z += displacement.z;

        glm::ivec3 minB = glm::ivec3(glm::floor(relPos + glm::vec3{playerWidthL + EPS, playerHeightU + EPS, playerLengthN}));
        glm::ivec3 maxB = glm::ivec3(glm::floor(relPos + glm::vec3{playerWidthR - EPS, playerHeightD - EPS, playerLengthF}));

        if (displacement.z < 0.0f) // w -Z
        {
            int z = minB.z;
            bool collided = false;
            for (int x = minB.x; x <= maxB.x && !collided; ++x)
            {
                for (int y = minB.y; y <= maxB.y; ++y)
                {
                    if (isSolid(x, y, z))
                    {
                        relPos.z = static_cast<float>(z + 1) - playerLengthN + EPS;
                        playerVelocity.z = 0.0f;
                        collided = true;
                        break;
                    }
                }
            }
        }
        else // w +Z
        {
            int z = maxB.z;
            bool collided = false;
            for (int x = minB.x; x <= maxB.x && !collided; ++x)
            {
                for (int y = minB.y; y <= maxB.y; ++y)
                {
                    if (isSolid(x, y, z))
                    {
                        relPos.z = static_cast<float>(z) - playerLengthF - EPS;
                        playerVelocity.z = 0.0f;
                        collided = true;
                        break;
                    }
                }
            }
        }
    }

    playerPosition = transform.translation + relPos;
}

void MppBlockChunk::calculateMesh()
{
    dirty = false;
    MppBlockModel::Builder builder;

    builder.vertices.reserve(10000);
    builder.indices.reserve(15000);

    for (size_t x{0}; x < CHUNK_SIZE; x++)
    {
        for (size_t y{0}; y < CHUNK_SIZE; y++)
        {
            for (size_t z{0}; z < CHUNK_SIZE; z++)
            {
                BlockID currentBlock = data[index(x, y, z)];
                if (currentBlock == BlockID::AIR || currentBlock == BlockID::BARRIER)
                {
                    continue;
                }

                glm::vec3 blockPos{static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};

                for (int dir = 0; dir < 6; dir++)
                {
                    int nx = static_cast<int>(x) + FACE_OFFSETS[dir].x;
                    int ny = static_cast<int>(y) + FACE_OFFSETS[dir].y;
                    int nz = static_cast<int>(z) + FACE_OFFSETS[dir].z;

                    if (isTransparent(nx, ny, nz))
                    {
                        uint32_t startVertexIndex = static_cast<uint32_t>(builder.vertices.size());

                        for (int i = 0; i < 4; i++)
                        {
                            MppBlockModel::Vertex v{};
                            v.pos = blockPos + FACE_VERTICES[dir][i];
                            v.normal = FACE_NORMALS[dir];
                            v.uv = FACE_UVS[i];
                            v.texID = static_cast<uint32_t>(currentBlock) + 1; // Tutaj możesz zmapować np. inną teksturę na górę trawy
                            builder.vertices.push_back(v);
                        }

                        builder.indices.push_back(startVertexIndex + 0);
                        builder.indices.push_back(startVertexIndex + 1);
                        builder.indices.push_back(startVertexIndex + 2);

                        builder.indices.push_back(startVertexIndex + 2);
                        builder.indices.push_back(startVertexIndex + 3);
                        builder.indices.push_back(startVertexIndex + 0);
                    }
                }
            }
        }
    }

    model.updateMesh(builder);
}



glm::vec3 MppBlockChunk::determinePlayerStartingPosition()
{
    auto checkFreeSpace = [&](size_t x, size_t y, size_t z){
        if(data[index(x, y, z)] == BlockID::AIR) return false;
        if(y == 0) return true;
        if(data[index(x, y-1, z)] != BlockID::AIR) return false;
        if(y-1 == 0) return true;
        if(data[index(x, y-2, z)] != BlockID::AIR) return false;
        return true;
    };


    for(size_t r{0}; r < CHUNK_SIZE/2; r++)
    {
        auto minr{CHUNK_SIZE/2 - 1 - r};
        auto maxr{CHUNK_SIZE/2 + r};


        if(checkFreeSpace(minr, minr, minr)) return transform.translation + glm::vec3{0.5f + minr, minr,0.5f + minr};
        for(size_t a{minr}; a < maxr; a++)
        {
            for(size_t b{minr}; b < maxr; b++)
            {
                if(checkFreeSpace(minr, a, b+1)) return transform.translation + glm::vec3{0.5f + minr, a,   0.5f + b+1 };
                if(checkFreeSpace(a+1, minr, b)) return transform.translation + glm::vec3{0.5f + a+1,  minr,0.5f + b   };
                if(checkFreeSpace(a, b+1, minr)) return transform.translation + glm::vec3{0.5f + a,    b+1, 0.5f + minr};

                if(checkFreeSpace(maxr, a+1, b)) return transform.translation + glm::vec3{0.5f + maxr, a+1, 0.5f + b   };
                if(checkFreeSpace(a, maxr, b+1)) return transform.translation + glm::vec3{0.5f + a,    maxr,0.5f + b+1 };
                if(checkFreeSpace(a+1, b, maxr)) return transform.translation + glm::vec3{0.5f + a+1,  b,   0.5f + maxr};
            }
        }
        if(checkFreeSpace(maxr, maxr, maxr)) return transform.translation + glm::vec3{0.5f + maxr, maxr,0.5f + maxr};
    }

    return transform.translation + glm::vec3{0.5f, 0.5f, 0.5f};
}




void MppBlockChunk::populateData()
{
    // patrząc w +Z X rośnie w prawo a Y w dół

    //podłoga z niezniszczalium
    for(size_t x{1}; x < CHUNK_SIZE - 1 ; x++)
    {
        for(size_t z{1}; z < CHUNK_SIZE -1; z++)
        {
            data[index(x,31,z)] = BlockID::UNBREAKABLEIUM;
        }
    }

    //ściany i sufit z barier
    for(size_t x{1}; x < CHUNK_SIZE - 1 ; x++)
    {
        for(size_t z{1}; z < CHUNK_SIZE - 1 ; z++)
        {
            data[index(x,0,z)] = BlockID::BARRIER;
        }
    }

    for(size_t x{1}; x< CHUNK_SIZE - 1 ; x++)
    {
        for(size_t y{1}; y< CHUNK_SIZE - 1 ; y++)
        {
            data[index(x,y,0)] = BlockID::BARRIER;
            data[index(x,y,31)] = BlockID::BARRIER;
        }
    }

    for(size_t y{1}; y< CHUNK_SIZE - 1 ; y++)
    {
        for(size_t z{0}; z< CHUNK_SIZE; z++)
        {
            data[index(0,y,z)] = BlockID::BARRIER;
            data[index(31,y,z)] = BlockID::BARRIER;
        }
    }

    //kamień
    for(size_t x{1}; x< CHUNK_SIZE - 1 ; x++)
    {
        for(size_t y{17}; y< CHUNK_SIZE - 1 ; y++)
        {
            for(size_t z{1}; z< CHUNK_SIZE - 1 ; z++)
            {
                data[index(x,y,z)] = BlockID::STONE;
            }
        }
    }

    //ziemia, trawa i nierówności
    for(size_t x{1}; x< CHUNK_SIZE - 1 ; x++)
    {
        for(size_t z{1}; z< CHUNK_SIZE - 1; z++)
        {
            float xyShift{40.0f};
            float scale{0.11f};
            float value = perlin(scale*x + xyShift,scale*z + xyShift);
            value += 1.0f;
            value /= 2.0f;
            value *= 3.7;
            size_t yShift{static_cast<size_t>(value)};

            if(yShift == 0)
            {
                data[index(x,19 - yShift,z)] = BlockID::SAND;
                data[index(x,18 - yShift,z)] = BlockID::SAND;
                data[index(x,17 - yShift,z)] = BlockID::SAND;
            }
            else
            {
                data[index(x,19 - yShift,z)] = BlockID::DIRT;
                data[index(x,18 - yShift,z)] = BlockID::DIRT;
                data[index(x,17 - yShift,z)] = BlockID::GRASS;
            }
        }
    }

    //drzewa
    for(size_t i{}; i< 4 ; i++)
    {
        int x = Random::get(2, CHUNK_SIZE - 3);
        int z = Random::get(2, CHUNK_SIZE - 3);
        int y{CHUNK_SIZE-1};
        while(data[index(x,y,z)] != BlockID::AIR)
        {
            y--;
        }

        data[index(x,y,z)] = BlockID::WOOD;
        data[index(x,y-1,z)] = BlockID::WOOD;
        data[index(x,y-2,z)] = BlockID::LEAF;
        data[index(x,y-3,z)] = BlockID::LEAF;
        data[index(x,y-2,z-1)] = BlockID::LEAF;
        data[index(x,y-2,z+1)] = BlockID::LEAF;
        data[index(x-1,y-2,z)] = BlockID::LEAF;
        data[index(x+1,y-2,z)] = BlockID::LEAF;
    }




}

} // namespace mpp::Block
