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
#include "ffengine/sim/fluid.hpp"

#include <algorithm>
#include <GL/glew.h>

#include "ffengine/io/log.hpp"
#include "ffengine/math/algo.hpp"

#include "ffengine/sim/fluid_native.hpp"

namespace sim {

/* sim::Fluid::Source */

Fluid::Source::Source(Object::ID object_id,
                      const Vector2f pos,
                      const float radius,
                      const float absolute_height,
                      const float capacity):
    Object(object_id),
    m_pos(pos),
    m_radius(radius),
    m_absolute_height(absolute_height),
    m_capacity(capacity)
{

}

Fluid::Source::Source(Object::ID object_id,
                      const float x, const float y,
                      const float radius,
                      const float absolute_height,
                      const float capacity):
    Source(object_id, Vector2f(x, y), radius, absolute_height, capacity)
{

}

/* sim::Fluid */

Fluid::Fluid(const Terrain &terrain):
    m_blocks((terrain.size()-1) / IFluidSim::block_size),
    m_impl(new NativeFluidSim(m_blocks, terrain)),
    m_sources_invalidated(false),
    m_terrain_update_conn(terrain.terrain_updated().connect(
                              sigc::mem_fun(*this, &Fluid::terrain_updated)))
{
    const unsigned int block_count = (terrain.size()-1) / IFluidSim::block_size;
    if (block_count * IFluidSim::block_size != terrain.size()-1) {
        throw std::runtime_error("Terrain size minus one must be a multiple of"
                                 " fluid block size, which is "+
                                 std::to_string(IFluidSim::block_size));
    }

    m_impl->set_ocean_level(0.f);
}

Fluid::~Fluid()
{
    m_terrain_update_conn.disconnect();
}

void Fluid::map_source(Source *obj)
{
    const Vector2f origin = obj->m_pos;
    const float radius = obj->m_radius;

    TerrainRect r(source_rect(obj));
    for (unsigned int y = r.y0(); y < r.y1(); ++y) {
        for (unsigned int x = r.x0(); x < r.x1(); ++x) {
            const float cell_dist = std::sqrt(sqr(x-origin[eX])+sqr(y-origin[eY]));
            if (cell_dist > radius) {
                continue;
            }

            FluidCellMeta *meta = m_blocks.cell_meta(x, y);
            meta->source_height = obj->m_absolute_height;
            meta->source_capacity = obj->m_capacity;

            m_blocks.block_for_cell(x, y)->set_active(true);
        }
    }
}

TerrainRect Fluid::source_rect(Source *obj) const
{
    const int ceil_radius = std::ceil(obj->m_radius);
    if (ceil_radius <= 0) {
        return TerrainRect(0, 0, 0, 0);
    }

    int x0 = std::round(obj->m_pos[eX])-ceil_radius;
    int y0 = std::round(obj->m_pos[eY])-ceil_radius;
    int x1 = std::round(obj->m_pos[eX])+ceil_radius;
    int y1 = std::round(obj->m_pos[eY])+ceil_radius;

    if (x0 < 0) {
        x0 = 0;
    } else if ((unsigned int)x0 >= m_blocks.cells_per_axis()) {
        x0 = m_blocks.cells_per_axis()-1;
    }
    if (x1 < 0) {
        x1 = 0;
    } else if ((unsigned int)x1 > m_blocks.cells_per_axis()) {
        x1 = m_blocks.cells_per_axis();
    }

    if (y0 < 0) {
        y0 = 0;
    } else if ((unsigned int)y0 >= m_blocks.cells_per_axis()) {
        y0 = m_blocks.cells_per_axis()-1;
    }
    if (y1 < 0) {
        y1 = 0;
    } else if ((unsigned int)y1 > m_blocks.cells_per_axis()) {
        y1 = m_blocks.cells_per_axis();
    }

    return TerrainRect(x0, y0, x1, y1);
}

void Fluid::terrain_updated(TerrainRect r)
{
    m_impl->terrain_update(r);
}

void Fluid::start()
{
    if (m_sources_invalidated) {
        for (Source *source: m_sources)
        {
            map_source(source);
        }
        m_sources_invalidated = false;
    }
    m_impl->start_frame();
}

/*void Fluid::to_gl_texture() const
{

}*/

void Fluid::wait_for()
{
    m_impl->wait_for_frame();
}

void Fluid::add_source(Source *obj)
{
    m_sources.emplace_back(obj);
    invalidate_sources();
}

void Fluid::invalidate_sources()
{
    m_sources_invalidated = true;
}

void Fluid::remove_source(Source *obj)
{
    auto iter = std::find(m_sources.begin(), m_sources.end(), obj);
    if (iter == m_sources.end()) {
        return;
    }

    unmap_source(obj);
    m_sources.erase(iter);
    invalidate_sources();
}

void Fluid::unmap_source(Source *obj)
{
    const Vector2f origin = obj->m_pos;
    const float radius = obj->m_radius;

    TerrainRect r(source_rect(obj));
    for (unsigned int y = r.y0(); y < r.y1(); ++y) {
        for (unsigned int x = r.x0(); x < r.x1(); ++x) {
            const float cell_dist = std::sqrt(sqr(x-origin[eX])+sqr(y-origin[eY]));
            if (cell_dist > radius) {
                continue;
            }

            FluidCellMeta *meta = m_blocks.cell_meta(x, y);
            meta->source_height = -1.f;
            meta->source_capacity = 0.f;

            m_blocks.block_for_cell(x, y)->set_active(true);
        }
    }

    invalidate_sources();
}


}
