/**********************************************************************
File name: world_ops.hpp
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
#ifndef SCC_SIM_WORLD_OPS_H
#define SCC_SIM_WORLD_OPS_H

#include "engine/sim/world.hpp"

namespace sim {
namespace ops {

class BrushWorldOperation: public WorldOperation
{
public:
    BrushWorldOperation(
            const float xc, const float yc,
            const unsigned int brush_size,
            const std::vector<float> &density_map,
            const float brush_strength);

protected:
    const float m_xc;
    const float m_yc;
    const unsigned int m_brush_size;
    const std::vector<float> m_density_map;
    const float m_brush_strength;

};


class ObjectWorldOperation: public WorldOperation
{
public:
    ObjectWorldOperation(
            const Object::ID object_id);

protected:
    const Object::ID m_object_id;

};


class TerraformRaise: public BrushWorldOperation
{
public:
    using BrushWorldOperation::BrushWorldOperation;

public:
    WorldOperationResult execute(WorldMutator &mutator) override;

};

class TerraformLevel: public BrushWorldOperation
{
public:
    TerraformLevel(
            const float xc, const float yc,
            const unsigned int brush_size,
            const std::vector<float> &density_map,
            const float brush_strength,
            const float reference_height
            );

private:
    const float m_reference_height;

public:
    WorldOperationResult execute(WorldMutator &mutator) override;

};

class FluidRaise: public BrushWorldOperation
{
public:
    using BrushWorldOperation::BrushWorldOperation;

public:
    WorldOperationResult execute(WorldMutator &mutator) override;

};

}
}

#endif
