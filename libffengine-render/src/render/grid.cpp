/**********************************************************************
File name: grid.cpp
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
#include "ffengine/render/grid.hpp"

namespace ffe {

GridNode::GridNode(Material &mat,
                   const unsigned int xcells,
                   const unsigned int ycells,
                   const float size):
    m_material(mat),
    m_vbo_alloc(m_material.vbo().allocate((xcells+1+ycells+1)*2)),
    m_ibo_alloc(m_material.ibo().allocate((xcells+1+ycells+1)*2))
{
    auto slice = VBOSlice<Vector3f>(m_vbo_alloc, 0);
    const float x0 = -size*xcells/2.;
    const float y0 = -size*ycells/2.;

    uint16_t *dest = m_ibo_alloc.get();
    unsigned int base = 0;
    for (unsigned int x = 0; x < xcells+1; x++)
    {
        slice[base+2*x] = Vector3f(x0+x*size, y0, 0);
        slice[base+2*x+1] = Vector3f(x0+x*size, -y0, 0);
        *dest++ = base+2*x;
        *dest++ = base+2*x+1;
    }

    base = (xcells+1)*2;
    for (unsigned int y = 0; y < ycells+1; y++)
    {
        slice[base+2*y] = Vector3f(x0, y0+y*size, 0);
        slice[base+2*y+1] = Vector3f(-x0, y0+y*size, 0);
        *dest++ = base+2*y;
        *dest++ = base+2*y+1;
    }

    m_vbo_alloc.mark_dirty();
    m_ibo_alloc.mark_dirty();

    m_material.sync_buffers();
}

void GridNode::render(RenderContext &context)
{
    context.render_all(AABB{}, GL_LINES, m_material, m_ibo_alloc, m_vbo_alloc);
}

void GridNode::sync(RenderContext &)
{

}

}
