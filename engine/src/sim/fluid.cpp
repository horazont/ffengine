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
#include "engine/sim/fluid.hpp"

#include <GL/glew.h>

#include "engine/gl/util.hpp"
#include "engine/io/log.hpp"

#include "engine/sim/fluid_native.hpp"

namespace sim {

static io::Logger &logger = io::logging().get_logger("sim.fluid");


Fluid::Fluid(const Terrain &terrain):
    m_blocks((terrain.size()-1) / IFluidSim::block_size),
    m_impl(new NativeFluidSim(m_blocks, terrain)),
    m_terrain_update_conn(terrain.terrain_updated().connect(
                              sigc::mem_fun(*this, &Fluid::terrain_updated)))
{
    const unsigned int block_count = (terrain.size()-1) / IFluidSim::block_size;
    if (block_count * IFluidSim::block_size != terrain.size()-1) {
        throw std::runtime_error("Terrain size minus one must be a multiple of"
                                 " fluid block size, which is "+
                                 std::to_string(IFluidSim::block_size));
    }
}

Fluid::~Fluid()
{
    m_terrain_update_conn.disconnect();
}

void Fluid::terrain_updated(TerrainRect r)
{
    m_impl->terrain_update(r);
}

void Fluid::start()
{
    m_impl->start_frame();
}

void Fluid::to_gl_texture() const
{
    const unsigned int total_cells = m_blocks.cells_per_axis()*m_blocks.cells_per_axis();

    // terrain_height, fluid_height, flowx, flowy
    std::vector<Vector4f> buffer(total_cells);

    {
        auto lock = m_blocks.read_frontbuffer();
        Vector4f *dest = &buffer.front();
        for (unsigned int y = 0; y < m_blocks.cells_per_axis(); ++y)
        {
            for (unsigned int x = 0; x < m_blocks.cells_per_axis(); ++x)
            {
                const FluidCellMeta *meta = m_blocks.cell_meta(x, y);
                const FluidCell *cell = m_blocks.cell_front(x, y);
                *dest = Vector4f(meta->terrain_height, cell->fluid_height,
                                 cell->fluid_flow[0], cell->fluid_flow[1]);

                ++dest;
            }
        }
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0,
                    m_blocks.cells_per_axis(),
                    m_blocks.cells_per_axis(),
                    GL_RGBA,
                    GL_FLOAT,
                    buffer.data());
    engine::raise_last_gl_error();
}

void Fluid::wait_for()
{
    m_impl->wait_for_frame();
}


}
