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

namespace ffe {

/* engine::QuadBezier3fDebug */

QuadBezier3fDebug::QuadBezier3fDebug(Octree &octree,
                                     Material &mat, unsigned int steps):
    OctNode(octree),
    m_mat(mat),
    m_steps(steps),
    m_curve_changed(true),
    m_vbo_alloc(mat.vbo().allocate(steps+2)),
    m_ibo_alloc(mat.ibo().allocate(steps+4))
{
    {
        uint16_t *dest = m_ibo_alloc.get();
        for (unsigned int step = 0; step <= steps; ++step) {
            *dest++ = step;
        }
        *dest++ = steps + 1;
        *dest++ = 0;
        *dest++ = steps;
        m_ibo_alloc.mark_dirty();
    }
    m_octree.insert_object(this);
}

void QuadBezier3fDebug::prepare(RenderContext &)
{

}

void QuadBezier3fDebug::render(RenderContext &context)
{
    context.render_all(AABB{}, GL_LINE_STRIP, m_mat, m_ibo_alloc, m_vbo_alloc);
}

void QuadBezier3fDebug::sync(scenegraph::OctContext &positioning)
{
    if (m_curve_changed) {
        Vector3f center((m_curve.p_start + m_curve.p_control + m_curve.p_end)/3.f);
        float radius = std::max((center - m_curve.p_start).length(),
                                std::max((center - m_curve.p_control).length(),
                                         (center - m_curve.p_end).length()));

        update_bounds(Sphere{center, radius});
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
        slice[m_steps+1] = Vector4f(m_curve.p_control, 0.5);
        m_vbo_alloc.mark_dirty();
        m_mat.sync_buffers();
    }
}

/* engine::QuadBezier3fRoadTest */

QuadBezier3fRoadTest::QuadBezier3fRoadTest(Octree &octree,
                                           Material &mat, unsigned int steps):
    OctNode(octree),
    m_mat(mat),
    m_steps(steps),
    m_curve_changed(true)
{
    octree.insert_object(this);
}

void QuadBezier3fRoadTest::prepare(RenderContext &)
{

}

void QuadBezier3fRoadTest::render(RenderContext &context)
{
    context.render_all(
                AABB{}, GL_TRIANGLES, m_mat, m_ibo_alloc, m_vbo_alloc);
}

void QuadBezier3fRoadTest::sync(scenegraph::OctContext &positioning)
{
    if (m_curve_changed) {
        Vector3f center((m_curve.p_start + m_curve.p_control + m_curve.p_end)/3.f);
        float radius = std::max((center - m_curve.p_start).length(),
                                std::max((center - m_curve.p_control).length(),
                                         (center - m_curve.p_end).length()));

        update_bounds(Sphere{center, radius});
    }

    if (m_curve_changed) {
        m_curve_changed = false;
        m_vbo_alloc = nullptr;
        m_ibo_alloc = nullptr;

        std::vector<float> ts;
        autosample_curve(m_curve, std::back_inserter(ts));

        m_steps = ts.size();

        m_ibo_alloc = m_mat.ibo().allocate(m_steps*6);
        m_vbo_alloc = m_mat.vbo().allocate((m_steps+1)*2);

        uint16_t *dest = m_ibo_alloc.get();
        for (unsigned int step = 0; step < m_steps; ++step) {
            *dest++ = 2*step;
            *dest++ = 2*step+1;
            *dest++ = 2*(step+1);

            *dest++ = 2*(step+1);
            *dest++ = 2*step+1;
            *dest++ = 2*(step+1)+1;
        }
        m_ibo_alloc.mark_dirty();

        const Vector3f up(0, 0, 1);

        auto slice = VBOSlice<Vector4f>(m_vbo_alloc, 0);
        for (unsigned int i = 0; i <= m_steps; ++i)
        {
            const float t = (float(i) / m_steps);
            const Vector3f p = m_curve[t];
            const Vector3f tangent = m_curve.diff(t).normalized();
            const Vector3f bitangent = (tangent % up).normalized();
            const Vector3f normal = (bitangent % tangent).normalized();

            const Vector3f left = p - bitangent;
            const Vector3f right = p + bitangent;

            slice[2*i] = Vector4f(
                        positioning.get_origin() + positioning.get_orientation().rotate(left),
                        t);
            slice[2*i+1] = Vector4f(
                        positioning.get_origin() + positioning.get_orientation().rotate(right),
                        t);
        }
        m_vbo_alloc.mark_dirty();
        m_mat.sync_buffers();
    }
}

}
