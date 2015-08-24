/**********************************************************************
File name: overlay.cpp
This file is part of: SCC (working title)

LICENSE

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

FEEDBACK & QUESTIONS

For feedback and questions about SCC please e-mail one of the authors named in
the AUTHORS file.
**********************************************************************/
#include "ffengine/render/overlay.hpp"

namespace engine {

OverlayNode::OverlayNode(Texture2D &depthbuffer):
    scenegraph::Node(),
    m_vbo(VBOFormat({
                        VBOAttribute(2)
                    })),
    m_depthbuffer(depthbuffer),
    m_vbo_alloc(m_vbo.allocate(4)),
    m_ibo_alloc(m_ibo.allocate(6)),
    m_min(0, 0),
    m_max(10, 10)
{
    {
        uint16_t *dest = m_ibo_alloc.get();
        *dest++ = 1;
        *dest++ = 0;
        *dest++ = 2;
        *dest++ = 3;
    }

    m_ibo_alloc.mark_dirty();

    bool success = m_material.shader().attach_resource(
                GL_VERTEX_SHADER,
                ":/shaders/overlay/main.vert");
    success = success && m_material.shader().attach_resource(
                GL_FRAGMENT_SHADER,
                ":/shaders/overlay/main.frag");
    success = success && m_material.shader().link();

    if (!success) {
        throw std::runtime_error("failed to compile or link shader");
    }

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);

    m_material.shader().bind();
    m_material.attach_texture("depth", &m_depthbuffer);
    {
        auto slice = VBOSlice<Vector2f>(m_vbo_alloc, 0);
        slice[0] = Vector2f(0, 0);
        slice[1] = Vector2f(0, 1);
        slice[2] = Vector2f(1, 1);
        slice[3] = Vector2f(1, 0);
    }
    m_vbo_alloc.mark_dirty();
}

void OverlayNode::render(RenderContext &context)
{
    m_material.shader().bind();
    glUniform2f(m_material.shader().uniform_location("viewport_size"),
                context.viewport_width(),
                context.viewport_height());
    glUniform2fv(m_material.shader().uniform_location("pmin"),
                 1, m_min.as_array);
    glUniform2fv(m_material.shader().uniform_location("pmax"),
                 1, m_max.as_array);
    glUniform1f(m_material.shader().uniform_location("znear"),
                context.znear());
    glUniform1f(m_material.shader().uniform_location("zfar"),
                context.zfar());
    glDisable(GL_DEPTH_TEST);
    context.draw_elements(GL_TRIANGLE_STRIP, *m_vao, m_material, m_ibo_alloc);
    glDisable(GL_DEPTH_TEST);
}

void OverlayNode::sync(RenderContext &)
{
    m_vao->sync();
}


}