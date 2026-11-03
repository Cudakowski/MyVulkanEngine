#pragma once

#include "mpp_model.hpp"
#include "mpp_texture.hpp"

#include <unordered_map>
#include <memory>

namespace mpp
{

struct TransformComponent{
    glm::vec3 translation{}; // {position offset}
    glm::vec3 scale{1.f,1.f, 1.f};
    glm::vec3 rotation{};

    glm::mat4 mat4();
    glm::mat3 normalMatrix();
};



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

    MppGameObject(const MppGameObject &)=delete;
    MppGameObject &operator=(const MppGameObject &) = delete;
    MppGameObject(MppGameObject &&) =default;
    MppGameObject &operator=(MppGameObject &&) = default;

    id_t getId(){return id;}

    TransformComponent transform{};
private:
    id_t id;

    MppGameObject(id_t newId): id{newId} {}

};




} // namespace mpp

