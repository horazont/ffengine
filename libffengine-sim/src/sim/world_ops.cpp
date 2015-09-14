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
#include "ffengine/sim/world_ops.hpp"

#include "ffengine/math/algo.hpp"


namespace sim {
namespace ops {


/**
 * Apply a terrain tool using a brush mask.
 *
 * @param field The heightfield to work on
 * @param brush_size Diameter of the brush
 * @param sampled Density map of the brush
 * @param brush_strength Factor which is applied to the density map for each
 * painted pixel.
 * @param terrain_size Size of the terrain, for clipping
 * @param x0 X center for painting
 * @param y0 Y center for painting
 * @param impl Tool implementation
 *
 * @see flatten_tool, raise_tool
 */
template <typename impl_t>
void apply_brush_masked_tool(sim::Terrain::HeightField &field,
                             const unsigned int brush_size,
                             const std::vector<float> &sampled,
                             const float brush_strength,
                             const unsigned int terrain_size,
                             const float x0,
                             const float y0,
                             const impl_t &impl)
{
    const int size = brush_size;
    const float radius = size / 2.f;
    const int terrain_xbase = std::round(x0 - radius);
    const int terrain_ybase = std::round(y0 - radius);

    for (int y = 0; y < size; y++) {
        const int yterrain = y + terrain_ybase;
        if (yterrain < 0) {
            continue;
        }
        if (yterrain >= (int)terrain_size) {
            break;
        }
        for (int x = 0; x < size; x++) {
            const int xterrain = x + terrain_xbase;
            if (xterrain < 0) {
                continue;
            }
            if (xterrain >= (int)terrain_size) {
                break;
            }

            sim::Terrain::height_t &h = field[yterrain*terrain_size+xterrain];
            h = std::max(sim::Terrain::min_height,
                         std::min(sim::Terrain::max_height,
                                  impl.paint(
                                      h, brush_strength*sampled[y*size+x],
                                      xterrain, yterrain)));
        }
    }
}

/**
 * Apply a fluid tool using a brush mask.
 *
 * @param field The fluid grid to work on
 * @param brush_size Diameter of the brush
 * @param sampled Density map of the brush
 * @param brush_strength Factor which is applied to the density map for each
 * painted pixel.
 * @param terrain_size Size of the terrain, for clipping
 * @param x0 X center for painting
 * @param y0 Y center for painting
 * @param impl Tool implementation
 *
 * @see flatten_tool, raise_tool
 */
template <typename impl_t>
void apply_brush_masked_tool(FluidBlocks &field,
                             const unsigned int brush_size,
                             const std::vector<float> &sampled,
                             const float brush_strength,
                             const float x0,
                             const float y0,
                             const impl_t &impl)
{
    const int fluid_size = field.cells_per_axis();
    const int size = brush_size;
    const float radius = size / 2.f;
    const int fluid_xbase = std::round(x0 - radius);
    const int fluid_ybase = std::round(y0 - radius);

    for (int y = 0; y < size; y++) {
        const int yfluid = y + fluid_ybase;
        if (yfluid < 0) {
            continue;
        }
        if (yfluid >= (int)fluid_size) {
            break;
        }
        for (int x = 0; x < size; x++) {
            const int xfluid = x + fluid_xbase;
            if (xfluid < 0) {
                continue;
            }
            if (xfluid >= (int)fluid_size) {
                break;
            }

            impl.apply(*field.cell_back(xfluid, yfluid),
                       brush_strength*sampled[y*size+x]);
            field.block_for_cell(xfluid, yfluid)->set_active(true);
        }
    }
}

/**
 * Tool implementation for raising / lowering the terrain based on a brush.
 *
 * For use with apply_brush_masked_tool().
 */
struct raise_tool
{
    sim::Terrain::height_t paint(sim::Terrain::height_t h,
                                 float brush_density,
                                 int, int) const
    {
        return h + brush_density;
    }
};

/**
 * Tool implementation for flattening the terrain based on a brush.
 *
 * For use with apply_brush_masked_tool().
 */
struct flatten_tool
{
    flatten_tool(sim::Terrain::height_t new_value):
        new_value(new_value)
    {

    }

    sim::Terrain::height_t new_value;

    sim::Terrain::height_t paint(sim::Terrain::height_t h,
                                 float brush_density,
                                 int, int) const
    {
        return interp_linear(h, new_value, brush_density);
    }
};

/**
 * Tool implementation for creating a ramp on the terrain
 *
 * For use with apply_brush_masked_tool().
 */
struct ramp_tool
{
    ramp_tool(const Vector2f source_point,
              const Terrain::height_t source_height,
              const Vector2f destination_point,
              const Terrain::height_t destination_height):
        origin(source_point),
        direction((destination_point - source_point).normalized()),
        length((destination_point - source_point).length()),
        source_height(source_height),
        destination_height(destination_height)
    {

    }

    Vector2f origin;
    Vector2f direction;
    float length;
    Terrain::height_t source_height, destination_height;

    sim::Terrain::height_t paint(sim::Terrain::height_t h,
                                 float brush_density,
                                 int x, int y) const
    {
        Vector2f p_in_origin_space = Vector2f(x, y) - origin;
        const float interp = clamp((p_in_origin_space * direction) / length,
                                   0.f, 1.f);
        return interp_linear(h, interp_linear(source_height, destination_height, interp), brush_density);
    }
};

struct fluid_raise_tool
{
    void apply(FluidCell &cell, float brush_density) const
    {
        cell.fluid_height = std::max(FluidFloat(0.), cell.fluid_height+brush_density);
    }
};


void notify_update_terrain_rect(const Terrain &terrain,
                                const float xc, const float yc,
                                const unsigned int brush_size)
{
    const float brush_radius = brush_size/2.f;
    sim::TerrainRect r(xc, yc, std::ceil(xc+brush_radius), std::ceil(yc+brush_radius));
    if (xc < std::ceil(brush_radius)) {
        r.set_x0(0);
    } else {
        r.set_x0(xc-std::ceil(brush_radius));
    }
    if (yc < std::ceil(brush_radius)) {
        r.set_y0(0);
    } else {
        r.set_y0(yc-std::ceil(brush_radius));
    }
    if (r.x1() > terrain.size()) {
        r.set_x1(terrain.size());
    }
    if (r.y1() > terrain.size()) {
        r.set_y1(terrain.size());
    }
    terrain.notify_heightmap_changed(r);
}


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

/* sim::ops::ObjectWorldOperation */

ObjectWorldOperation::ObjectWorldOperation(const Object::ID object_id):
    m_object_id(object_id)
{

}

/* sim::ops::TerraformRaise */

WorldOperationResult TerraformRaise::execute(WorldState &state)
{
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = state.terrain().writable_field(field);
        apply_brush_masked_tool(*field,
                                m_brush_size, m_density_map, m_brush_strength,
                                state.terrain().size(),
                                m_xc, m_yc,
                                raise_tool());
    }
    notify_update_terrain_rect(state.terrain(), m_xc, m_yc, m_brush_size);
    return NO_ERROR;
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

WorldOperationResult TerraformLevel::execute(WorldState &state)
{
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = state.terrain().writable_field(field);
        apply_brush_masked_tool(*field,
                                m_brush_size, m_density_map, m_brush_strength,
                                state.terrain().size(),
                                m_xc, m_yc,
                                flatten_tool(m_reference_height));
    }
    notify_update_terrain_rect(state.terrain(), m_xc, m_yc, m_brush_size);
    return NO_ERROR;
}


/* sim::ops::TerraformSmooth */

Terrain::height_t TerraformSmooth::sample_parzen_rect(
        const Terrain::HeightField &field,
        const unsigned int terrain_size,
        const unsigned int xc, const unsigned int yc,
        const unsigned int size)
{
    TerrainRect r(0, 0, xc+size, yc+size);
    if (xc < size) {
        r.set_x0(0);
    } else {
        r.set_x0(xc-size);
    }
    if (yc < size) {
        r.set_y0(0);
    } else {
        r.set_y0(yc-size);
    }

    if (r.x1() > terrain_size) {
        r.set_x1(terrain_size);
    }
    if (r.y1() > terrain_size) {
        r.set_y1(terrain_size);
    }


    float total_weight = 0;
    float total_weighted_height = 0;
    for (unsigned int y = r.y0(); y < r.y1(); ++y) {
        for (unsigned int x = r.x0(); x < r.x1(); ++x) {
            const float d = std::sqrt(
                        sqr((float)x-(float)xc)+sqr((float)y-(float)yc)) / size;
            const float weight = parzen(d);
            total_weight += weight;
            Terrain::height_t height = field[y*terrain_size+x];
            total_weighted_height += height*weight;
        }
    }

    if (total_weight == 0) {
        return NAN;
    }

    return total_weighted_height / total_weight;
}

WorldOperationResult TerraformSmooth::execute(WorldState &state)
{
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = state.terrain().writable_field(field);

        // we cannot use apply_brush_masked_tool here, because we need
        // information about our surroundings
        const int size = m_brush_size;
        const float radius = size / 2.f;
        const int terrain_xbase = std::round(m_xc - radius);
        const int terrain_ybase = std::round(m_yc - radius);
        const unsigned int terrain_size = state.terrain().size();

        for (int y = 0; y < size; y++) {
            const int yterrain = y + terrain_ybase;
            if (yterrain < 0) {
                continue;
            }
            if (yterrain >= (int)terrain_size) {
                break;
            }
            for (int x = 0; x < size; x++) {
                const int xterrain = x + terrain_xbase;
                if (xterrain < 0) {
                    continue;
                }
                if (xterrain >= (int)terrain_size) {
                    break;
                }

                Terrain::height_t &h = (*field)[yterrain*terrain_size+xterrain];

                Terrain::height_t new_h = sample_parzen_rect(
                            *field, terrain_size, xterrain, yterrain, 3);
                if (isnan(new_h)) {
                    continue;
                }

                h = std::max(
                            sim::Terrain::min_height,
                            std::min(
                                sim::Terrain::max_height,
                                interp_linear(
                                    h, new_h,
                                    m_brush_strength*m_density_map[y*size+x])));
            }
        }
    }
    notify_update_terrain_rect(state.terrain(), m_xc, m_yc, m_brush_size);
    return NO_ERROR;
}


/* sim::ops::TerraformRamp */

TerraformRamp::TerraformRamp(const float xc, const float yc,
                             const unsigned int brush_size,
                             const std::vector<float> &density_map,
                             const float brush_strength,
                             const Vector2f source_point,
                             const Terrain::height_t source_height,
                             const Vector2f destination_point,
                             const Terrain::height_t destination_height):
    BrushWorldOperation(xc, yc, brush_size, density_map, brush_strength),
    m_source_point(source_point),
    m_source_height(source_height),
    m_destination_point(destination_point),
    m_destination_height(destination_height)
{

}

WorldOperationResult TerraformRamp::execute(WorldState &state)
{
    if ((m_destination_point - m_source_point).length() < 1) {
        return INVALID_ARGUMENT;
    }
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = state.terrain().writable_field(field);
        apply_brush_masked_tool(*field,
                                m_brush_size, m_density_map, m_brush_strength,
                                state.terrain().size(),
                                m_xc, m_yc,
                                ramp_tool(m_source_point, m_source_height,
                                          m_destination_point, m_destination_height));
    }
    notify_update_terrain_rect(state.terrain(), m_xc, m_yc, m_brush_size);
    return NO_ERROR;
}


/* sim::ops::FluidRaise */

WorldOperationResult FluidRaise::execute(WorldState &state)
{
    apply_brush_masked_tool(state.fluid().blocks(),
                            m_brush_size, m_density_map,
                            m_brush_strength,
                            m_xc, m_yc,
                            fluid_raise_tool());
    return NO_ERROR;
}


/* sim::ops::FluidSourceCreate */

FluidSourceCreate::FluidSourceCreate(const float x, const float y,
        const float radius,
        const float height,
        const float capacity,
        const Object::ID object_id):
    ObjectWorldOperation(object_id),
    m_x(x),
    m_y(y),
    m_radius(radius),
    m_height(height),
    m_capacity(capacity)
{

}

WorldOperationResult FluidSourceCreate::execute(WorldState &state)
{
    Fluid::Source &obj = state.objects().allocate<Fluid::Source>(
                m_x, m_y,
                m_radius,
                m_height,
                m_capacity);
    state.fluid().add_source(&obj);
    return NO_ERROR;
}

WorldOperationResult FluidSourceDestroy::execute(WorldState &state)
{
    Fluid::Source *obj = state.objects().get_safe<Fluid::Source>(m_object_id);
    if (!obj) {
        return NO_SUCH_OBJECT;
    }

    state.fluid().remove_source(obj);
    return NO_ERROR;
}


}
}
