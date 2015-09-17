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
#include "ffengine/render/plane.hpp"

#include "ffengine/render/renderpass.hpp"


namespace ffe {

/* ffe::PlaneNode */

PlaneNode::PlaneNode(const Plane &plane, Material &material, const float size):
    m_plane(plane),
    m_size(size),
    m_plane_changed(true),
    m_material(material),
    m_ibo_alloc(material.ibo().allocate(12)),
    m_vbo_alloc(material.vbo().allocate(4))
{
    {
        // front and back
        uint16_t *dest = m_ibo_alloc.get();
        *dest++ = 0;
        *dest++ = 1;
        *dest++ = 2;

        *dest++ = 2;
        *dest++ = 1;
        *dest++ = 3;

        *dest++ = 0;
        *dest++ = 2;
        *dest++ = 1;

        *dest++ = 1;
        *dest++ = 2;
        *dest++ = 3;
    }
    m_ibo_alloc.mark_dirty();
}

void PlaneNode::set_plane(const Plane &plane)
{
    m_plane = plane;
    m_plane_changed = true;
}

void PlaneNode::prepare(RenderContext &)
{

}

void PlaneNode::render(RenderContext &context)
{
    context.render_all(AABB(), GL_TRIANGLES,
                       m_material,
                       m_ibo_alloc,
                       m_vbo_alloc);
}

void PlaneNode::sync()
{
    if (m_plane_changed) {
        auto positions = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        auto normals = VBOSlice<Vector3f>(m_vbo_alloc, 1);

        const Vector3f normal(m_plane.normal());
        const Vector3f origin(m_plane.origin());

        for (unsigned int i = 0; i < 3; ++i) {
            normals[i] = normal;
        }

        Vector3f u(-normal[eY],  normal[eX],          0);
        Vector3f v(          0, -normal[eZ], normal[eY]);
        u.normalize();
        v.normalize();
        /*std::cout << u << " " << v << std::endl;*/
        if (u == Vector3f()) {
            u = Vector3f(-normal[eZ], 0, normal[eX]);
            u.normalize();
        } else if (v == Vector3f()) {
            v = Vector3f(-normal[eZ], 0, normal[eX]);
            v.normalize();
        } else if (std::fabs(u*v - 1.f) < 0.00001f) {
            v = Vector3f(-normal[eZ], 0, normal[eX]);
            v.normalize();
        }

        positions[0] = origin - u * m_size - v * m_size;
        positions[1] = origin + u * m_size - v * m_size;
        positions[2] = origin - u * m_size + v * m_size;
        positions[3] = origin + u * m_size + v * m_size;
        m_vbo_alloc.mark_dirty();
        m_material.sync_buffers();
        m_plane_changed = false;
    }
}



}
