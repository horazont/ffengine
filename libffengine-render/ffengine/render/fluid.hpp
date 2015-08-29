/**********************************************************************
File name: fluid.hpp
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
#ifndef SCC_ENGINE_RENDER_FLUID_H
#define SCC_ENGINE_RENDER_FLUID_H

#include "ffengine/sim/fluid.hpp"

#include "ffengine/render/scenegraph.hpp"


namespace engine {

/**
 * Render the frontbuffer of a sim::Fluid simulation.
 */
class FluidNode: public scenegraph::Node
{
public:
    FluidNode(sim::Fluid &fluidsim);

private:
    sim::Fluid &m_fluidsim;
    double m_t;

    Texture2D m_fluiddata;

    Material m_material;

    VBOAllocation m_vbo_alloc;
    IBOAllocation m_ibo_alloc;

    std::vector<Vector4f> m_transfer_buffer;

protected:
    void fluidsim_to_gl_texture();

public:
    void attach_waves_texture(Texture2D *tex);
    void attach_scene_colour_texture(Texture2D *tex);
    void attach_scene_depth_texture(Texture2D *tex);

public:
    void advance(TimeInterval seconds) override;
    void render(RenderContext &context) override;
    void sync(RenderContext &context) override;

};

}

#endif // SCC_ENGINE_RENDER_GRID_H

