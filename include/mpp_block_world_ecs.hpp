#pragma once

#include "mpp_block_chunk.hpp"

namespace mpp
{



class MppBlockWorldECS
{
private:
    
public:
    MppBlockWorldECS(/* args */);
    ~MppBlockWorldECS();

    //ustaw pod swoje potrzeby
    MppBlockWorldECS(const MppBlockWorldECS&) = delete;
    MppBlockWorldECS& operator=(const MppBlockWorldECS&) = delete;
    MppBlockWorldECS(MppBlockWorldECS&&) = delete;
    MppBlockWorldECS&& operator=(MppBlockWorldECS&&) = delete;
};

} // namespace mpp
