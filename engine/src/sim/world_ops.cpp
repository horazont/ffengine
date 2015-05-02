/**********************************************************************
File name: world_ops.cpp
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
#include "engine/sim/world_ops.hpp"

namespace sim {
namespace ops {

/* sim::ops::BrushWorldOperation */

BrushWorldOperation::BrushWorldOperation(
        const float xc, const float yc,
        const unsigned int brush_size,
        const std::vector<float> &density_map,
        const float brush_strength):
    m_xc(xc),
    m_yc(yc),
    m_brush_size(brush_size),
    m_density_map(density_map),
    m_brush_strength(brush_strength)
{

}

/* sim::ops::TerraformRaise */

WorldOperationResult TerraformRaise::execute(WorldMutator &mutator)
{
    return mutator.tf_raise(m_xc, m_yc,
                            m_brush_size,
                            m_density_map,
                            m_brush_strength);
}


/* sim::ops::TerraformLevel */

TerraformLevel::TerraformLevel(
        const float xc, const float yc,
        const unsigned int brush_size,
        const std::vector<float> &density_map,
        const float brush_strength,
        const float reference_height):
    BrushWorldOperation(xc, yc, brush_size, density_map, brush_strength),
    m_reference_height(reference_height)
{

}

WorldOperationResult TerraformLevel::execute(WorldMutator &mutator)
{
    return mutator.tf_level(m_xc, m_yc,
                            m_brush_size,
                            m_density_map,
                            m_brush_strength,
                            m_reference_height);
}


/* sim::ops::FluidRaise */

WorldOperationResult FluidRaise::execute(WorldMutator &mutator)
{
    return mutator.fluid_raise(m_xc, m_yc,
                               m_brush_size,
                               m_density_map,
                               m_brush_strength);
}


}
}
