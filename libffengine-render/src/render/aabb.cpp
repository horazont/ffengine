#include "ffengine/render/aabb.hpp"

namespace engine {

DynamicAABBs::DynamicAABBs(DiscoverCallback &&cb):
    scenegraph::Node(),
    m_discover_cb(std::move(cb)),
    m_vbo(VBOFormat({
                        VBOAttribute(4)
                    }))
{
    bool success = m_material.shader().attach_resource(
                GL_VERTEX_SHADER,
                ":/shaders/aabb/main.vert");
    success = success && m_material.shader().attach_resource(
                GL_FRAGMENT_SHADER,
                ":/shaders/aabb/main.frag");
    success = success && m_material.shader().link();

    if (!success) {
        throw std::runtime_error("failed to compile or link shader");
    }

    ArrayDeclaration decl;
    decl.declare_attribute("position_t", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);
}

void DynamicAABBs::render(RenderContext &context)
{
    context.draw_elements_base_vertex_less(GL_LINES, *m_vao, m_material, m_ibo_alloc,
                                           m_vbo_alloc.base(),
                                           m_aabbs.size()*24);
}

void DynamicAABBs::sync(RenderContext &)
{
    m_discover_cb(m_aabbs);

    const unsigned int boxes = m_aabbs.size();
    const unsigned int vertices = boxes * 8;

    if (!m_vbo_alloc || m_vbo_alloc.length() < vertices)
    {
        // reallocate
        m_vbo_alloc = nullptr;
        m_ibo_alloc = nullptr;

        m_vbo_alloc = m_vbo.allocate(vertices);
        m_ibo_alloc = m_ibo.allocate(boxes*24);

        uint16_t *dest = m_ibo_alloc.get();
        for (unsigned int i = 0; i < boxes; ++i)
        {
            *dest++ = i*8;
            *dest++ = i*8+1;

            *dest++ = i*8+1;
            *dest++ = i*8+3;

            *dest++ = i*8+3;
            *dest++ = i*8+2;

            *dest++ = i*8+2;
            *dest++ = i*8;

            *dest++ = i*8;
            *dest++ = i*8+4;

            *dest++ = i*8+1;
            *dest++ = i*8+5;

            *dest++ = i*8+2;
            *dest++ = i*8+6;

            *dest++ = i*8+3;
            *dest++ = i*8+7;

            *dest++ = i*8+4;
            *dest++ = i*8+5;

            *dest++ = i*8+5;
            *dest++ = i*8+7;

            *dest++ = i*8+7;
            *dest++ = i*8+6;

            *dest++ = i*8+6;
            *dest++ = i*8+4;
        }
        m_ibo_alloc.mark_dirty();
        m_ibo.sync();
    }

    auto slice = VBOSlice<Vector4f>(m_vbo_alloc, 0);
    unsigned int index = 0;
    for (const auto &box: m_aabbs)
    {
        slice[index++] = Vector4f(box.min, 0.f);
        slice[index++] = Vector4f(box.min[eX], box.min[eY], box.max[eZ], 1.f);
        slice[index++] = Vector4f(box.min[eX], box.max[eY], box.min[eZ], 1.f);
        slice[index++] = Vector4f(box.min[eX], box.max[eY], box.max[eZ], 0.f);
        slice[index++] = Vector4f(box.max[eX], box.min[eY], box.min[eZ], 1.f);
        slice[index++] = Vector4f(box.max[eX], box.min[eY], box.max[eZ], 0.f);
        slice[index++] = Vector4f(box.max[eX], box.max[eY], box.min[eZ], 0.f);
        slice[index++] = Vector4f(box.max, 1.f);
    }
    m_vbo_alloc.mark_dirty();
    m_vbo.sync();
}

}
