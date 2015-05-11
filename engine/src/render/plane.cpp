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

ZUpPlaneNode::ZUpPlaneNode(const float width, const float height,
                           const unsigned int cells):
    m_vbo(VBOFormat({
                        VBOAttribute(3)
                    })),
    m_ibo(),
    m_material(),
    m_vbo_alloc(m_vbo.allocate((cells+1)*(cells+1))),
    m_ibo_alloc(m_ibo.allocate(cells*cells*4))
{
    {
        auto slice = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        unsigned int i = 0;
        for (unsigned int y = 0; y <= cells; y++)
        {
            const float yf = float(y) / cells * 2.f - 1.f;
            for (unsigned int x = 0; x <= cells; x++)
            {
                const float xf = float(x) / cells * 2.f - 1.f;
                slice[i] = Vector3f(xf*width/2.f, yf*height/2.f, 0);
                ++i;
            }
        }
    }

    {
        uint16_t *dest = m_ibo_alloc.get();
        for (unsigned int y = 0; y < cells; y++) {
            for (unsigned int x = 0; x < cells; x++) {
                const unsigned int curr_base = y*(cells+1) + x;
                *dest++ = curr_base;
                *dest++ = curr_base + (cells+1);
                *dest++ = curr_base + (cells+1) + 1;
                *dest++ = curr_base + 1;
            }
        }
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
    /*glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
    m_material.bind();
    glUniform3fv(m_material.shader().uniform_location("viewpoint"),
                 1,
                 context.viewpoint().as_array);
    context.draw_elements(GL_LINES_ADJACENCY, *m_vao, m_material, m_ibo_alloc);
    /*glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);*/
}

void ZUpPlaneNode::sync(RenderContext &)
{
    m_vao->sync();
}

}
