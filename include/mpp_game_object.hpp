#pragma once

#include "mpp_model.hpp"
#include "mpp_texture.hpp"

#include <unordered_map>
#include <memory>

namespace mpp
{


class MppGameObject 
{

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
private:
    id_t id;

    MppGameObject(id_t newId): id{newId} {}

};




} // namespace mpp

