#ifndef SCC_ENGINE_RENDER_FRUSTUM_H
#define SCC_ENGINE_RENDER_FRUSTUM_H

#include "engine/render/scenegraph.hpp"

namespace engine {

class FrustumNode: public scenegraph::Node
{
public:
    FrustumNode();

private:
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
