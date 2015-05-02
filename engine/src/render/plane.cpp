/**********************************************************************
File name: plane.cpp
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
#include "engine/render/plane.hpp"

namespace engine {

ZUpPlaneNode::ZUpPlaneNode(const float width, const float height):
    m_vbo(VBOFormat({
                        VBOAttribute(3)
                    })),
    m_ibo(),
    m_material(),
    m_vbo_alloc(m_vbo.allocate(4)),
    m_ibo_alloc(m_ibo.allocate(4))
{
    {
        auto slice = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        slice[0] = Vector3f(0, 0, 0);
        slice[1] = Vector3f(0, height, 0);
        slice[2] = Vector3f(width, height, 0);
        slice[3] = Vector3f(width, 0, 0);
    }

    {
        uint16_t *dest = m_ibo_alloc.get();
        *dest++ = 1;
        *dest++ = 0;
        *dest++ = 2;
        *dest++ = 3;
    }

    m_vbo_alloc.mark_dirty();
    m_ibo_alloc.mark_dirty();

}

Material &ZUpPlaneNode::material()
{
    return m_material;
}

void ZUpPlaneNode::setup_vao()
{
    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);

    RenderContext::configure_shader(m_material.shader());
}

void ZUpPlaneNode::render(RenderContext &context)
{
    context.draw_elements(GL_TRIANGLE_STRIP, *m_vao, m_material, m_ibo_alloc);
}

void ZUpPlaneNode::sync(RenderContext &)
{
    m_vao->sync();
}

}
