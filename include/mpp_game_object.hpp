#pragma once

#include "mpp_model.hpp"
#include "mpp_texture.hpp"

#include <unordered_map>

namespace mpp
{


class MppGameObject 
{
private:
    id_t id;

    MppGameObject(id_t newId): id{newId} {}
public:
    using id_t  = unsigned int;
    using Map = std::unordered_map<id_t, MppGameObject>;

    std::shared_ptr<MppModel> model{};
    std::shared_ptr<MppTexture> texture{};

    static MppGameObject createGameObject()
    {
        static id_t currentId = 0; 
        return MppGameObject{currentId++};
    }

    id_t getId(){return id;}
};




} // namespace mpp

