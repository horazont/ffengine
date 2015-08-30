/**********************************************************************
File name: curve.cpp
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
#include "ffengine/render/curve.hpp"

namespace engine {

QuadBezier3fDebug::QuadBezier3fDebug(Material &mat, unsigned int steps):
    m_mat(mat),
    m_steps(steps),
    m_vbo_alloc(mat.vbo().allocate(steps+1)),
    m_ibo_alloc(mat.ibo().allocate(steps+1))
{
    {
        const uint16_t base = m_vbo_alloc.base();
        uint16_t *dest = m_ibo_alloc.get();
        for (unsigned int step = 0; step <= steps; ++step) {
            *dest++ = base + step;
        }
        m_ibo_alloc.mark_dirty();
    }
}

void QuadBezier3fDebug::render(RenderContext &context)
{
    context.draw_elements(GL_LINE_STRIP, m_mat, m_ibo_alloc);
}

void QuadBezier3fDebug::sync(RenderContext &context,
                             ffe::Octree &octree,
                             scenegraph::OctContext &positioning)
{
    if (m_curve_changed || !this->octree()) {
        Vector3f center((m_curve.p1 + m_curve.p2 + m_curve.p3)/3.f);
        float radius = std::max((center - m_curve.p1).length(),
                                std::max((center - m_curve.p2).length(),
                                         (center - m_curve.p3).length()));

        update_bounds(Sphere{center, radius});
        if (!this->octree()) {
            octree.insert_object(this);
        }
    }

    if (m_curve_changed) {
        auto slice = VBOSlice<Vector4f>(m_vbo_alloc, 0);
        for (unsigned int i = 0; i <= m_steps; ++i)
        {
            const float t = (float(i) / m_steps);
            const Vector3f p = m_curve[t];
            slice[i] = Vector4f(
                        positioning.get_origin() + positioning.get_orientation().rotate(p),
                        t);
        }
        m_vbo_alloc.mark_dirty();
        m_mat.sync();
    }
}

}
