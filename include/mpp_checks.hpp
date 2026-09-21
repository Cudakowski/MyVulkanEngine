#pragma once

#include <vulkan/vulkan.h>

#include <iostream>

namespace mpp
{

// static inline void chk(VkResult result);
// static inline void chk(bool result);

static inline void chk(VkResult result) {
    if(result != VK_SUCCESS){
        std::cerr << "Vulkan call returned an error (" << result << ")\n";
        exit(result);
    }
}
static inline void chk(bool result) {
    if(!result){
        std::cerr << "Call returned an error\n";
        exit(result);
    }
}

} // namespace mpp
