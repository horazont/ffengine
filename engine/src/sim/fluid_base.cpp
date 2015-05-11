/**********************************************************************
File name: fluid_base.cpp
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
#include "engine/sim/fluid_base.hpp"

namespace sim {

const FluidFloat IFluidSim::flow_damping = 0.995;
/* const FluidFloat Fluid::flow_damping = 0.1; */
const FluidFloat IFluidSim::flow_friction = 0.6;
const unsigned int IFluidSim::block_size = 60;

/* sim::IFluidSim */

IFluidSim::~IFluidSim()
{

}

/* sim::FluidCellMeta */

FluidCellMeta::FluidCellMeta():
    terrain_height(0),
    source_height(-1),
    source_capacity(0)
{

}


FluidCell::FluidCell():
    fluid_height(0.f),
    fluid_flow{0.f, 0.f}
{

}

/* sim::FluidBlock */

const FluidFloat FluidBlock::CHANGE_BACKLOG_FILTER_CONSTANT = 0.9f;
const FluidFloat FluidBlock::CHANGE_BACKLOG_THRESHOLD  = 0.0001f;
const FluidFloat FluidBlock::REACTIVATION_THRESHOLD = 0.00012f;
const FluidFloat FluidBlock::CHANGE_TRANSFER_FACTOR = 1.f;


FluidBlock::FluidBlock(const unsigned int x,
                       const unsigned int y):
    m_x(x),
    m_y(y),
    m_change_backlog(0.f),
    m_front_change(0.f),
    m_active(false),
    m_meta_cells(IFluidSim::block_size*IFluidSim::block_size),
    m_back_cells(m_meta_cells.size()),
    m_front_cells(m_meta_cells.size())
{
    // set_active has side effects which we leverage here
    set_active(true);
}

/* sim::FluidBlocks */

FluidBlocks::FluidBlocks(const unsigned int block_count_per_axis):
    m_blocks_per_axis(block_count_per_axis),
    m_cells_per_axis(IFluidSim::block_size*m_blocks_per_axis),
    m_blocks()
{
    m_blocks.reserve(m_blocks_per_axis*m_blocks_per_axis);
    for (unsigned int y = 0; y < m_blocks_per_axis; ++y)
    {
        for (unsigned int x = 0; x < m_blocks_per_axis; ++x)
        {
            m_blocks.emplace_back(x, y);
        }
    }
}

}
