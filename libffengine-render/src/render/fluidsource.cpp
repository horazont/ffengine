/**********************************************************************
File name: fluidsource.cpp
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
#include "ffengine/render/fluidsource.hpp"

#include "ffengine/math/intersect.hpp"


namespace ffe {

/* ffe::FluidSourceMaterial */

FluidSourceMaterial::FluidSourceMaterial(unsigned int resolution):
    Material(VBOFormat({VBOAttribute(3), VBOAttribute(3)})),
    m_vertices(resolution+3),
    m_vbo_alloc(vbo().allocate(m_vertices*4+2)), // four vertices per edge
                                                 // (different normals for side and cap)
    m_ibo_alloc(ibo().allocate(m_vertices*4*3)) // four triangles per edge
{
    {
        uint16_t *dest = m_ibo_alloc.get();
        for (unsigned int i = 0; i < m_vertices; ++i)
        {
            const unsigned int vertex_base = 2 + i*4;
            const unsigned int next_vertex_base = 2 + ((i+1) % m_vertices)*4;
            *dest++ = 0;
            *dest++ = vertex_base;
            *dest++ = next_vertex_base;

            *dest++ = vertex_base+1;
            *dest++ = vertex_base+2;
            *dest++ = next_vertex_base+1;

            *dest++ = next_vertex_base+1;
            *dest++ = vertex_base+2;
            *dest++ = next_vertex_base+2;

            *dest++ = next_vertex_base+3;
            *dest++ = vertex_base+3;
            *dest++ = 1;
        }
    }
    m_ibo_alloc.mark_dirty();

    {
        auto positions = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        auto normals = VBOSlice<Vector3f>(m_vbo_alloc, 1);

        const Vector3f top_normal(0, 0, 1);
        const Vector3f bottom_normal(0, 0, -1);

        positions[0] = Vector3f(0, 0, 1);
        normals[0] = top_normal;
        positions[1] = Vector3f(0, 0, 0);
        normals[1] = bottom_normal;

        for (unsigned int i = 0; i < m_vertices; ++i)
        {
            const unsigned int vertex_base = 2 + i*4;

            const float alpha = float(i) / float(m_vertices) * M_PI * 2;
            const float sin_alpha = sin(alpha);
            const float cos_alpha = cos(alpha);

            const Vector3f top(cos_alpha, sin_alpha, 1);
            const Vector3f bottom(cos_alpha, sin_alpha, 0);

            const Vector3f side_normal(cos_alpha, sin_alpha, 0);

            positions[vertex_base] = top;
            normals[vertex_base] = top_normal;

            positions[vertex_base+1] = top;
            normals[vertex_base+1] = side_normal;

            positions[vertex_base+2] = bottom;
            normals[vertex_base+2] = side_normal;

            positions[vertex_base+3] = bottom;
            normals[vertex_base+3] = bottom_normal;
        }
    }
    m_vbo_alloc.mark_dirty();

    declare_attribute("position", 0);
    declare_attribute("normal", 1);

    sync_buffers();
}

/* ffe::FluidSource */

FluidSource::FluidSource(FluidSourceMaterial &mat):
    m_mat(mat),
    m_state(UI_STATE_INACTIVE),
    m_base(0, 0),
    m_radius(0),
    m_height(0),
    m_metrics_changed(true)
{

}

void FluidSource::set_base(const Vector2f &base)
{
    if (m_metrics_changed || m_base != base) {
        m_base = base;
        m_metrics_changed = true;
    }
}

void FluidSource::set_capacity(const float capacity)
{
    m_capacity = capacity;
}

void FluidSource::set_height(const float height)
{
    if (m_metrics_changed || m_height != height) {
        m_height = height;
        m_metrics_changed = true;
    }
}

void FluidSource::set_radius(const float radius)
{
    if (m_metrics_changed || m_radius != radius) {
        m_radius = radius;
        m_metrics_changed = true;
    }
}

void FluidSource::set_ui_state(const UIState state)
{
    m_state = state;
}

void FluidSource::update_from_source(const sim::Fluid::Source &source)
{
    if (source.m_pos != m_base ||
            source.m_absolute_height != m_height ||
            source.m_radius != m_radius) {
        m_base = source.m_pos;
        m_height = source.m_absolute_height;
        m_radius = source.m_radius;
        m_metrics_changed = true;
    }
    m_capacity = source.m_capacity;
}

void FluidSource::sync(Octree &octree, scenegraph::OctContext &positioning)
{
    if (this->octree() != &octree) {
        if (this->octree()) {
            this->octree()->remove_object(this);
        }
    }

    if (m_metrics_changed) {
        const Vector3f v1(m_radius, 0, m_height);
        const Vector3f v2(-m_radius, 0, 0);
        const float sphere_radius = (v2 - v1).length();

        const Vector3f origin = positioning.get_origin() + Vector3f(m_base, m_height/2);

        std::cout << "metrics changed: " << origin << " radius " << sphere_radius << std::endl;

        update_bounds(Sphere{origin, sphere_radius});

        m_metrics_changed = false;
    }

    if (this->octree() != &octree) {
        octree.insert_object(this);
    }

    switch (m_state)
    {
    case UI_STATE_INACTIVE:
    {
        m_add_colour = Vector4f(0, 0, 0, 0);
        break;
    }
    case UI_STATE_HOVER:
    {
        m_add_colour = Vector4f(0.1, 0.1, 0.1, 0.1);
        break;
    }
    case UI_STATE_SELECTED:
    {
        m_add_colour = Vector4f(0.1, 0.1, 0.1, 0);
        break;
    }
    }
    m_state = UI_STATE_INACTIVE;
}

void FluidSource::prepare(RenderContext &context)
{

}

void FluidSource::render(RenderContext &context)
{
    context.render_all(AABB{Vector3f(-m_radius, -m_radius, 0), Vector3f(m_radius, m_radius, m_height)},
                       GL_TRIANGLES,
                       m_mat,
                       m_mat.ibo_alloc(),
                       m_mat.vbo_alloc(),
                       [this](MaterialPass &pass){
                           glUniform1f(pass.shader().uniform_location("radius"), m_radius);
                           glUniform1f(pass.shader().uniform_location("height"), m_height);
                           glUniform1f(pass.shader().uniform_location("capacity"), m_capacity);
                           glUniform2fv(pass.shader().uniform_location("pos"), 1, m_base.as_array);
                           glUniform4fv(pass.shader().uniform_location("add_colour"), 1, m_add_colour.as_array);
                       });
}

bool FluidSource::isect_ray(const Ray &ray, float &tmin) const
{
    float _;
    return isect_cylinder_ray(Vector3f(m_base, 0), Vector3f(0, 0, m_height), m_radius, ray, tmin, _);
}

}
