#include "engine/render/pointer.hpp"


namespace engine {

PointerNode::PointerNode(const float radius):
    scenegraph::Node(),
    m_vbo(VBOFormat({
                        VBOAttribute(3)
                    })),
    m_vbo_alloc(m_vbo.allocate(8)),
    m_ibo_alloc(m_ibo.allocate(36))
{
    {
        auto slice = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        slice[0] = Vector3f(-radius, -radius, -radius);
        slice[1] = Vector3f(radius, -radius, -radius);
        slice[2] = Vector3f(radius, radius, -radius);
        slice[3] = Vector3f(-radius, radius, -radius);

        slice[4] = Vector3f(-radius, -radius, radius);
        slice[5] = Vector3f(-radius, radius, radius);
        slice[6] = Vector3f(radius, radius, radius);
        slice[7] = Vector3f(radius, -radius, radius);
    }

    {
        uint16_t *dest = m_ibo_alloc.get();
        // bottom
        *dest++ = 1;
        *dest++ = 0;
        *dest++ = 2;

        *dest++ = 2;
        *dest++ = 0;
        *dest++ = 3;

        // back
        *dest++ = 0;
        *dest++ = 1;
        *dest++ = 4;

        *dest++ = 4;
        *dest++ = 1;
        *dest++ = 7;

        // right
        *dest++ = 2;
        *dest++ = 6;
        *dest++ = 1;

        *dest++ = 1;
        *dest++ = 6;
        *dest++ = 7;

        // front
        *dest++ = 3;
        *dest++ = 5;
        *dest++ = 2;

        *dest++ = 2;
        *dest++ = 5;
        *dest++ = 6;

        // left
        *dest++ = 4;
        *dest++ = 5;
        *dest++ = 0;

        *dest++ = 0;
        *dest++ = 5;
        *dest++ = 3;

        // top
        *dest++ = 4;
        *dest++ = 7;
        *dest++ = 5;

        *dest++ = 5;
        *dest++ = 7;
        *dest++ = 6;
    }

    m_vbo_alloc.mark_dirty();
    m_ibo_alloc.mark_dirty();

    bool success = m_material.shader().attach(
                GL_VERTEX_SHADER,
                "#version 330\n"
                "layout(std140) uniform MatrixBlock {"
                "  layout(row_major) mat4 proj;"
                "  layout(row_major) mat4 view;"
                "  layout(row_major) mat4 model;"
                "  layout(row_major) mat3 normal;"
                "};"
                "in vec3 position;"
                "void main() {"
                "  gl_Position = proj * view * model * vec4(position, 1.f);"
                "}");

    success = success && m_material.shader().attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "out vec4 color;"
                "void main() {"
                "  color = vec4(0.8, 0.9, 1.0, 0.8);"
                "}");

    success = success && m_material.shader().link();

    if (!success) {
        throw std::runtime_error("failed to link shader");
    }

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);

    RenderContext::configure_shader(m_material.shader());
}

void PointerNode::render(RenderContext &context)
{
    context.draw_elements(GL_TRIANGLES, *m_vao, m_material, m_ibo_alloc);
}

void PointerNode::sync()
{
    m_vao->sync();
}

}
