#include "basic_render_system.hpp"

namespace mpp
{
BasicRenderSystem::BasicRenderSystem(MppDevice &device, MppRenderer &renderer, MppLoader &loader): mppDevice{device}, mppRenderer{renderer}, mppLoader{loader}
{
    createPipeline();
}

BasicRenderSystem::~BasicRenderSystem()
{

}

void BasicRenderSystem::createPipeline()
{
    mppPipeline = std::make_unique<MppPipeline>(
        mppDevice,
        mppLoader,
        mppRenderer
    );
}


} // namespace mpp
