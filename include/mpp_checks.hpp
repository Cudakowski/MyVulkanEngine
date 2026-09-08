#pragma once

#include <vulkan/vulkan.h>

namespace mpp
{

static inline void chk(VkResult result);
static inline void chk(bool result);

} // namespace mpp
