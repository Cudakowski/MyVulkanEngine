#include "mpp_renderer.hpp"

namespace mpp
{


MppRenderer::MppRenderer(MppDevice& device, MppWindow& window): mppDevice{device}, mppWindow{window}
{
    mppSwapChain = std::make_unique<MppSwapChain>(mppDevice, mppWindow);
}
    
} // namespace mpp
