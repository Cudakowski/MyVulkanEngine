#pragma once

#include "mpp_model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "mpp_checks.hpp"

#include <vector>
#include <memory>
#include <string>

namespace mpp
{

class MppLoader
{
private:

public:
    MppLoader();
    ~MppLoader();

    std::unique_ptr<MppModel> createModelFromFile(const std::string &file);

};




} // namespace mpp


