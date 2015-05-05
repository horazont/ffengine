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
#ifndef SCC_SIM_FLUID_H
#define SCC_SIM_FLUID_H

#include "engine/sim/terrain.hpp"

#include "engine/common/utils.hpp"

#include "engine/sim/fluid_base.hpp"

namespace sim {

/**
 * Fluid simulation.
 *
 * The fluid simulation is a huge fun project.
 */
class Fluid
{
public:
    Fluid(const Terrain &terrain);
    ~Fluid();

private:
    FluidBlocks m_blocks;
    std::unique_ptr<IFluidSim> m_impl;

    /* owned by Fluid */
    sigc::connection m_terrain_update_conn;

protected:
    void terrain_updated(TerrainRect r);

public:
    inline FluidBlocks &blocks()
    {
        return m_blocks;
    }

    void start();
    void to_gl_texture() const;
    void wait_for();

};

}

#endif
