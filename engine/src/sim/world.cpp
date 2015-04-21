/**********************************************************************
File name: world.cpp
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
#include "engine/sim/world.hpp"

#include "engine/math/algo.hpp"


namespace sim {

/* utilities */

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
                                  impl.paint(h, brush_strength*sampled[y*size+x])));
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
                                 float brush_density) const
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
                                 float brush_density) const
    {
        return interp_linear(h, new_value, brush_density);
    }
};


/* sim::TerraformWorld */

TerraformWorld::TerraformWorld():
    m_terrain(1081)
{

}

void TerraformWorld::notify_update_terrain_rect(const float xc,
                                                const float yc,
                                                unsigned int brush_size)
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
    if (r.x1() > m_terrain.size()) {
        r.set_x1(m_terrain.size());
    }
    if (r.y1() > m_terrain.size()) {
        r.set_y1(m_terrain.size());
    }

    m_terrain.notify_heightmap_changed(r);
}

const Terrain &TerraformWorld::terrain() const
{
    return m_terrain;
}

void TerraformWorld::tf_raise(const float xc, const float yc,
                              const unsigned int brush_size,
                              const std::vector<float> &density_map,
                              const float brush_strength)
{
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = m_terrain.writable_field(field);
        apply_brush_masked_tool(*field,
                                brush_size, density_map, brush_strength,
                                m_terrain.size(),
                                xc, yc,
                                raise_tool());
    }
    notify_update_terrain_rect(xc, yc, brush_size);
}

void TerraformWorld::tf_level(const float xc, const float yc,
                              const unsigned int brush_size,
                              const std::vector<float> &density_map,
                              const float brush_strength,
                              const float ref_height)
{
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = m_terrain.writable_field(field);
        apply_brush_masked_tool(*field,
                                brush_size, density_map, brush_strength,
                                m_terrain.size(),
                                xc, yc,
                                flatten_tool(ref_height));
    }
    notify_update_terrain_rect(xc, yc, brush_size);
}

}
