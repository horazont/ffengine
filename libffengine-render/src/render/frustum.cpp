/**********************************************************************
File name: frustum.cpp
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
#include "ffengine/render/frustum.hpp"


namespace ffe {

FrustumNode::FrustumNode():
    scenegraph::Node(),
    m_material(VBOFormat({
                        VBOAttribute(3)
                    })),
    m_vbo_alloc(m_material.vbo().allocate(16)),
    m_ibo_alloc(m_material.ibo().allocate(4*2*3))
{
    uint16_t *dest = m_ibo_alloc.get();
    for (unsigned int face = 0; face < 4; face++)
    {
        const unsigned int base = face*4;
        *dest++ = base+0;
        *dest++ = base+1;
        *dest++ = base+2;

        *dest++ = base+2;
        *dest++ = base+1;
        *dest++ = base+3;
    }
    m_ibo_alloc.mark_dirty();

    bool success = m_material.shader().attach_resource(
                GL_VERTEX_SHADER,
                ":/shaders/frustum/main.vert");
    success = success && m_material.shader().attach_resource(
                GL_FRAGMENT_SHADER,
                ":/shaders/frustum/main.frag");

    m_material.declare_attribute("position", 0);

    success = success && m_material.link();

    if (!success) {
        throw std::runtime_error("failed to compile or link shader");
    }

    m_material.sync();
}

void FrustumNode::render(RenderContext &context)
{
    glDisable(GL_CULL_FACE);
    context.draw_elements(GL_TRIANGLES, m_material, m_ibo_alloc);
    glEnable(GL_CULL_FACE);
}

void FrustumNode::sync(RenderContext &context)
{
    const float plane_size = 100.f;
    {
        const std::array<Plane, 6> &frustum = context.frustum();
        auto slice = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        for (unsigned int plane = 0; plane < 4; plane++)
        {
            const Vector3f n = Vector3f(frustum[plane].homogenous);
            const Vector3f origin = n * frustum[plane].homogenous[eW];
            /*std::cout << n << std::endl;*/
            Vector3f u(-n[eY],  n[eX],     0);
            Vector3f v(     0, -n[eZ], n[eY]);
            u.normalize();
            v.normalize();
            /*std::cout << u << " " << v << std::endl;*/
            if (u == Vector3f()) {
                u = Vector3f(-n[eZ], 0, n[eX]);
                u.normalize();
            } else if (v == Vector3f()) {
                v = Vector3f(-n[eZ], 0, n[eX]);
                v.normalize();
            } else if (fabs(u*v - 1.f) < 0.00001f) {
                v = Vector3f(-n[eZ], 0, n[eX]);
                v.normalize();
            }
            /*std::cout << u << " " << v << std::endl;*/

            for (unsigned int t1 = 0; t1 < 2; t1++) {
                const float t1f = float(t1*2*plane_size)-plane_size;
                for (unsigned int t2 = 0; t2 < 2; t2++) {
                    const float t2f = float(t2*2*plane_size)-plane_size;
                    slice[plane*4+(t1*2)+t2] = origin + u*t1f + v*t2f;
                    /*std::cout << plane*4+(t1*2)+t2 << " " << slice[plane*4+(t1*2)+t2] << std::endl;*/
                }
            }
        }
    }
    m_vbo_alloc.mark_dirty();

    /* _Exit(1); */

    m_material.sync();
}

}
