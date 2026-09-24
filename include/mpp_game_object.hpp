#pragma once

#include "mpp_model.hpp"

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

    static MppGameObject createGameObject()
    {
        static id_t currentId = 0; 
        return MppGameObject{currentId++};
    }

    id_t getId(){return id;}
};




} // namespace mpp

