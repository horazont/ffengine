/**********************************************************************
File name: pointer.cpp
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
#include "ffengine/render/pointer.hpp"


namespace ffe {

PointerNode::PointerNode(Material &mat, const float radius):
    scenegraph::Node(),
    m_material(mat),
    m_vbo_alloc(m_material.vbo().allocate(8)),
    m_ibo_alloc(m_material.ibo().allocate(36))
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

    m_material.sync_buffers();
}

void PointerNode::render(RenderContext &context)
{
    context.render_all(AABB{}, GL_TRIANGLES, m_material, m_ibo_alloc, m_vbo_alloc);
}

void PointerNode::sync(RenderContext &)
{

}

}
