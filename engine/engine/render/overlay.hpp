#ifndef SCC_ENGINE_RENDER_OVERLAY_H
#define SCC_ENGINE_RENDER_OVERLAY_H

#include "engine/render/scenegraph.hpp"

namespace engine {

/**
 * THIS STUFF DOES NOT WORK!
 */
class OverlayNode: public scenegraph::Node
{
public:
    OverlayNode(Texture2D &depthbuffer);

private:
    engine::VBO m_vbo;
    engine::IBO m_ibo;
    engine::Material m_material;

    Texture2D &m_depthbuffer;

    std::unique_ptr<engine::VAO> m_vao;

    engine::VBOAllocation m_vbo_alloc;
    engine::IBOAllocation m_ibo_alloc;

    Vector2f m_min, m_max;

public:
    void set_rectangle(const Vector2f &min,
                       const Vector2f &max);

public:
    void render(RenderContext &context) override;
    void sync(Scene &scene) override;

};

}

#endif
