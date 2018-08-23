#include "ffengine/render/fullscreenquad.hpp"

namespace ffe {

static io::Logger &logger = io::logging().get_logger("render");

FullScreenQuadNode::FullScreenQuadNode():
    m_material(VBOFormat({ffe::VBOAttribute(2)})),
    m_ibo_alloc(m_material.ibo().allocate(6)),
    m_vbo_alloc(m_material.vbo().allocate(4))
{
    m_material.declare_attribute("position", 0);

    {
        // front and back
        uint16_t *dest = m_ibo_alloc.get();
        *dest++ = 0;
        *dest++ = 1;
        *dest++ = 2;

        *dest++ = 2;
        *dest++ = 1;
        *dest++ = 3;
    }
    m_ibo_alloc.mark_dirty();

    {
        auto positions = VBOSlice<Vector2f>(m_vbo_alloc, 0);
        positions[0] = Vector2f(-1, -1);
        positions[1] = Vector2f(1, -1);
        positions[2] = Vector2f(-1, 1);
        positions[3] = Vector2f(1, 1);
    }
    m_vbo_alloc.mark_dirty();
}

void FullScreenQuadNode::prepare(RenderContext &)
{
    /* if (!m_linked) {
        if (!m_material.link()) {
            logger.logf(io::LOG_ERROR,
                        "failed to link full screen quad material!");
            return;
        }
        m_linked = true;
    } */
}

void FullScreenQuadNode::render(RenderContext &context)
{
    context.render_all(AABB(), GL_TRIANGLES,
                       m_material,
                       m_ibo_alloc,
                       m_vbo_alloc);
}

void FullScreenQuadNode::sync()
{
    m_material.sync_buffers();
}

}
