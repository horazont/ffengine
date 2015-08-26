#ifndef SCC_ENGINE_RENDER_AABB_H
#define SCC_ENGINE_RENDER_AABB_H

#include <functional>
#include <vector>

#include "ffengine/math/shapes.hpp"

#include "ffengine/render/scenegraph.hpp"

namespace engine {

class DynamicAABBs: public scenegraph::Node
{
public:
    using DiscoverCallback = std::function<void(std::vector<AABB>&)>;

public:
    DynamicAABBs(DiscoverCallback &&cb);

private:
    std::vector<AABB> m_aabbs;
    DiscoverCallback m_discover_cb;

    engine::VBO m_vbo;
    engine::IBO m_ibo;
    engine::Material m_material;
    std::unique_ptr<engine::VAO> m_vao;

    engine::VBOAllocation m_vbo_alloc;
    engine::IBOAllocation m_ibo_alloc;

public:
    void render(RenderContext &context) override;
    void sync(RenderContext &context) override;

};

}

#endif
