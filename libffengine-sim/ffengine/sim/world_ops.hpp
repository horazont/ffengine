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

#include "ffengine/math/curve.hpp"

#include "ffengine/sim/world.hpp"


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
    explicit ObjectWorldOperation(const Object::ID object_id);

protected:
    const Object::ID m_object_id;

};


/**
 * Raise the terrain around \a xc, \a yc.
 *
 * This uses the given brush, determined by the \a brush_size and the
 * \a density_map, multiplied with \a brush_strength. \a brush_strength
 * may be negative to create a lowering effect.
 *
 * @param xc X center of the effect
 * @param yc Y center of the effect
 * @param brush_size Diameter of the brush
 * @param density_map Density values of the brush (must have \a brush_size
 * times \a brush_size entries).
 * @param brush_strength Strength factor for applying the brush, should be
 * in the range [-1.0, 1.0].
 */
class TerraformRaise: public BrushWorldOperation
{
public:
    using BrushWorldOperation::BrushWorldOperation;

public:
    WorldOperationResult execute(WorldState &state) override;

};

/**
 * Level the terrain around \a xc, \a yc to a specific reference height
 * \a ref_height.
 *
 * This uses the given brush, determined by the \a brush_size and the
 * \a density_map, multiplied with \a brush_strength. Using a negative
 * brush strength will have the same effect as using a negative
 * \a ref_height and will lead to clipping on the lower end of the terrain
 * height dynamic range.
 *
 * @param xc X center of the effect
 * @param yc Y center of the effect
 * @param brush_size Diameter of the brush
 * @param density_map Density values of the brush (must have \a brush_size
 * times \a brush_size entries).
 * @param brush_strength Strength factor for applying the brush, should be
 * in the range [0.0, 1.0].
 * @param ref_height Reference height to level the terrain to.
 */
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
    WorldOperationResult execute(WorldState &state) override;

};

class TerraformSmooth: public BrushWorldOperation
{
public:
    using BrushWorldOperation::BrushWorldOperation;

protected:
    Terrain::height_t sample_parzen_rect(
            const Terrain::Field &field,
            const unsigned int terrain_size,
            const unsigned int xc, const unsigned int yc,
            const unsigned int size);

public:
    WorldOperationResult execute(WorldState &state) override;

};

class TerraformRamp: public BrushWorldOperation
{
public:
    TerraformRamp(
            const float xc, const float yc,
            const unsigned int brush_size,
            const std::vector<float> &density_map,
            const float brush_strength,
            const Vector2f source_point,
            const Terrain::height_t source_height,
            const Vector2f destination_point,
            const Terrain::height_t destination_height);

private:
    const Vector2f m_source_point;
    const Terrain::height_t m_source_height;
    const Vector2f m_destination_point;
    const Terrain::height_t m_destination_height;

public:
    WorldOperationResult execute(WorldState &state) override;

};


class FluidRaise: public BrushWorldOperation
{
public:
    using BrushWorldOperation::BrushWorldOperation;

public:
    WorldOperationResult execute(WorldState &state) override;

};


class FluidSourceCreate: public ObjectWorldOperation
{
public:
    FluidSourceCreate(
            const float x, const float y,
            const float radius,
            const float height,
            const float capacity,
            const Object::ID object_id = Object::NULL_OBJECT_ID);

private:
    const float m_x;
    const float m_y;
    const float m_radius;
    const float m_height;
    const float m_capacity;

public:
    WorldOperationResult execute(WorldState &state) override;

public:
    static std::unique_ptr<FluidSourceCreate> from_source(
            Fluid::Source *source);

};


class FluidSourceMove: public ObjectWorldOperation
{
public:
    FluidSourceMove(
            const Object::ID object_id,
            const float new_x,
            const float new_y);

private:
    const float m_new_x;
    const float m_new_y;

public:
    WorldOperationResult execute(WorldState &state) override;

};


class FluidSourceSetHeight: public ObjectWorldOperation
{
public:
    FluidSourceSetHeight(
            const Object::ID object_id,
            const float new_absolute_height);

private:
    const float m_new_absolute_height;

public:
    WorldOperationResult execute(WorldState &state) override;

};


class FluidSourceSetCapacity: public ObjectWorldOperation
{
public:
    FluidSourceSetCapacity(
            const Object::ID object_id,
            const float new_capacity);

private:
    const float m_new_capacity;

public:
    WorldOperationResult execute(WorldState &state) override;

};


class FluidSourceDestroy: public ObjectWorldOperation
{
public:
    using ObjectWorldOperation::ObjectWorldOperation;

public:
    WorldOperationResult execute(WorldState &state) override;

};


class FluidOceanLevelSetHeight: public WorldOperation
{
public:
    FluidOceanLevelSetHeight(const float new_height);

private:
    const float m_new_height;

public:
    WorldOperationResult execute(WorldState &state) override;

};


class FluidReset: public WorldOperation
{
public:
    WorldOperationResult execute(WorldState &state) override;

};


class ConstructNewCurve: public ObjectWorldOperation
{
public:
    ConstructNewCurve(const Vector3f &start_point,
                      const sim::object_ptr<sim::PhysicalNode> &start_node,
                      const Vector3f &control_point,
                      const Vector3f &end_point,
                      const sim::object_ptr<sim::PhysicalNode> &end_node,
                      Object::ID new_object_id = Object::NULL_OBJECT_ID);

private:
    const sim::object_ptr<sim::PhysicalNode> m_end_node;
    const Vector3f m_end_point;
    const Vector3f m_control_point;
    const sim::object_ptr<sim::PhysicalNode> m_start_node;
    const Vector3f m_start_point;

public:
    WorldOperationResult execute(WorldState &state) override;

};


}
}

#endif
