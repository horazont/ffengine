/**********************************************************************
File name: fluid.cpp
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
#include "engine/render/fluid.hpp"

#define TIMELOG_FLUID_RENDER

#ifdef TIMELOG_FLUID_RENDER
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;

#define TIMELOG_ms(x) std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(x).count()
#endif

namespace engine {

static io::Logger &logger = io::logging().get_logger("render.fluid");

const unsigned int cells = 120;
const unsigned int width = 120;
const unsigned int height = 120;

FluidNode::FluidNode(sim::Fluid &fluidsim):
    m_fluidsim(fluidsim),
    m_fluiddata(GL_RGBA32F,
                fluidsim.blocks().cells_per_axis(),
                fluidsim.blocks().cells_per_axis(),
                GL_RGBA,
                GL_FLOAT),
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

    bool success = m_material.shader().attach_resource(
                GL_VERTEX_SHADER,
                ":/shaders/fluid/main.vert");
    success = success && m_material.shader().attach_resource(
                GL_GEOMETRY_SHADER,
                ":/shaders/fluid/main.geom");
    success = success && m_material.shader().attach_resource(
                GL_FRAGMENT_SHADER,
                ":/shaders/fluid/main.frag");
    success = success && m_material.shader().link();
    if (!success) {
        throw std::runtime_error("shader failed to compile or link");
    }

    m_material.shader().bind();
    glUniform1f(m_material.shader().uniform_location("width"),
                m_fluidsim.blocks().cells_per_axis());
    glUniform1f(m_material.shader().uniform_location("height"),
                m_fluidsim.blocks().cells_per_axis());

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);

    RenderContext::configure_shader(m_material.shader());

    m_fluiddata.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    m_material.attach_texture("fluiddata", &m_fluiddata);

    raise_last_gl_error();
}

void FluidNode::fluidsim_to_gl_texture()
{
    const unsigned int block_cells = sim::IFluidSim::block_size*sim::IFluidSim::block_size;

#ifdef TIMELOG_FLUID_RENDER
    timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_upload;
#endif

    // terrain_height, fluid_height, flowx, flowy
    m_transfer_buffer.resize(block_cells);

    sim::FluidBlocks &blocks = m_fluidsim.blocks();
    {
        auto lock = blocks.read_frontbuffer();
        for (unsigned int y = 0; y < blocks.blocks_per_axis(); ++y)
        {
            for (unsigned int x = 0; x < blocks.blocks_per_axis(); ++x)
            {
                const sim::FluidBlock *block = blocks.block(x, y);
                if (!block->active()) {
                    continue;
                }

                Vector4f *dest = &m_transfer_buffer.front();
                const sim::FluidCellMeta *meta = block->local_cell_meta(0, 0);
                const sim::FluidCell *cell = block->local_cell_front(0, 0);
                for (unsigned int i = 0; i < block_cells; ++i) {
                    *dest = Vector4f(meta->terrain_height, cell->fluid_height,
                                     cell->fluid_flow[0], cell->fluid_flow[1]);

                    ++dest;
                    ++meta;
                    ++cell;
                }
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                                x*sim::IFluidSim::block_size,
                                y*sim::IFluidSim::block_size,
                                sim::IFluidSim::block_size,
                                sim::IFluidSim::block_size,
                                GL_RGBA,
                                GL_FLOAT,
                                m_transfer_buffer.data());
            }
        }
    }
    raise_last_gl_error();

#ifdef TIMELOG_FLUID_RENDER
    t_upload = timelog_clock::now();
    logger.logf(io::LOG_DEBUG, "fluid: texture upload: %.2f ms",
                TIMELOG_ms(t_upload - t0));
#endif
}

void FluidNode::attach_waves_texture(Texture2D *tex)
{
    m_material.attach_texture("waves", tex);
}

void FluidNode::attach_scene_colour_texture(Texture2D *tex)
{
    m_material.attach_texture("scene", tex);
}

void FluidNode::attach_scene_depth_texture(Texture2D *tex)
{
    m_material.attach_texture("scene_depth", tex);
}

void FluidNode::advance(TimeInterval seconds)
{
    m_t += seconds;
}

void FluidNode::render(RenderContext &context)
{
    /*glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
    m_material.bind();
    glUniform3fv(m_material.shader().uniform_location("viewpoint"),
                 1,
                 context.viewpoint().as_array);
    glUniform2f(m_material.shader().uniform_location("viewport"),
                context.viewport_width(), context.viewport_height());
    context.draw_elements(GL_LINES_ADJACENCY, *m_vao, m_material, m_ibo_alloc);
    /*glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);*/
}

void FluidNode::sync(RenderContext &)
{
    m_vao->sync();
    m_material.shader().bind();
    glUniform1f(m_material.shader().uniform_location("t"),
                m_t);
    m_fluiddata.bind();
    fluidsim_to_gl_texture();
}

}
