/**********************************************************************
File name: fluidsource.hpp
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
#ifndef SCC_RENDER_FLUIDSOURCE_H
#define SCC_RENDER_FLUIDSOURCE_H

#include "ffengine/sim/fluid.hpp"

#include "ffengine/render/scenegraph.hpp"
#include "ffengine/render/renderpass.hpp"


namespace ffe {


enum UIState
{
    UI_STATE_INACTIVE,
    UI_STATE_HOVER,
    UI_STATE_SELECTED
};


class FluidSourceMaterial: public Material
{
public:
    FluidSourceMaterial(unsigned int resolution);

private:
    unsigned int m_vertices;
    VBOAllocation m_vbo_alloc;
    IBOAllocation m_ibo_alloc;

public:
    inline IBOAllocation &ibo_alloc()
    {
        return m_ibo_alloc;
    }

    inline VBOAllocation &vbo_alloc()
    {
        return m_vbo_alloc;
    }

};


class FluidSource: public RenderableOctreeObject, public scenegraph::OctNode
{
public:
    FluidSource(Octree &octree, FluidSourceMaterial &mat);

private:
    FluidSourceMaterial &m_mat;

    const sim::Fluid::Source *m_source;

    UIState m_state;
    Vector2f m_base;
    float m_radius;
    float m_height;
    float m_capacity;

    bool m_metrics_changed;

    Vector4f m_add_colour;

public:
    inline const sim::Fluid::Source *source() const
    {
        return m_source;
    }

    void set_base(const Vector2f &base);
    void set_capacity(const float capacity);
    void set_height(const float height);
    void set_radius(const float radius);
    void set_source(const sim::Fluid::Source *source);
    void set_ui_state(const UIState state);
    void update_from_source();

public: // OctNode interface
    void sync(scenegraph::OctContext &positioning) override;

public: // RenderableOctreeObject interface
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;

public: // OctreeObject interface
    bool isect_ray(const Ray &ray, float &tmin) const;

};

}

#endif
