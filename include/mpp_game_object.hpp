#pragma once

#include "mpp_model.hpp"

// std
#include <unordered_map>
#include <memory>

namespace mpp
{

struct TransformComponent{
    glm::vec3 translation{}; // {position offset}
    glm::vec3 scale{1.f,1.f, 1.f};
    glm::vec3 rotation{};

    glm::mat4 mat4()const;
    glm::mat3 normalMatrix()const;
};



class MppGameObject 
{

public:
    using id_t  = unsigned int;
    using Map = std::unordered_map<id_t, MppGameObject>;

    std::shared_ptr<MppModel> model{};
    uint32_t textureID{};//to na razie nic nie robi, docelowo obiekt powinien mieć materiał/tablice materiałów i to te materiały powinny mieć tekstury

    static MppGameObject createGameObject()
    {
        static id_t currentId = 0; 
        return MppGameObject{currentId++};
    }

    MppGameObject(const MppGameObject &)=delete;
    MppGameObject &operator=(const MppGameObject &) = delete;
    MppGameObject(MppGameObject &&) =default;
    MppGameObject &operator=(MppGameObject &&) = default;

    id_t getId()const{return id;}

    TransformComponent transform{};
private:
    id_t id;

    MppGameObject(id_t newId): id{newId} {}

};




} // namespace mpp

