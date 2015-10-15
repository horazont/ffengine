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
    m_terrain_update_conn(terrain.heightmap_updated().connect(
                              sigc::mem_fun(*this, &Fluid::terrain_updated)))
{
    const unsigned int block_count = (terrain.size()-1) / IFluidSim::block_size;
    if (block_count * IFluidSim::block_size != terrain.size()-1) {
        throw std::runtime_error("Terrain size minus one must be a multiple of"
                                 " fluid block size, which is "+
                                 std::to_string(IFluidSim::block_size));
    }

    set_ocean_level(0.f);
}

Fluid::~Fluid()
{
    m_terrain_update_conn.disconnect();
}

void Fluid::copy_from_block(Vector4f *dest,
                            const FluidBlock &src,
                            const unsigned int x0,
                            const unsigned int y0,
                            const unsigned int width,
                            const unsigned int height,
                            const unsigned int row_stride,
                            const unsigned int step) const
{
    for (unsigned int y = y0; y < y0 + height; y += step) {
        const sim::FluidCell *cell = src.local_cell_front(x0, y);
        const sim::FluidCellMeta *meta = src.local_cell_meta(x0, y);

        for (unsigned int x = x0; x < x0 + width; x += step) {
            *dest++ = Vector4f(
                        meta->terrain_height,
                        cell->fluid_height,
                        cell->fluid_flow[0],
                        cell->fluid_flow[1]);
            cell += step;
            meta += step;
        }

        dest += row_stride;
    }
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

void Fluid::set_ocean_level(const float level)
{
    m_ocean_level = level;
    m_impl->set_ocean_level(level);
}

void Fluid::reset()
{
    m_blocks.reset(m_ocean_level);
    invalidate_sources();
}

void Fluid::copy_block(Vector4f *dest,
                       const unsigned int x0,
                       const unsigned int y0,
                       const unsigned int width,
                       const unsigned int height,
                       const unsigned int oversample,
                       const unsigned int dest_width,
                       bool &used_active) const
{
    const unsigned int oversampled_width = width * oversample;
    const unsigned int oversampled_height = height * oversample;

    unsigned int ybase = y0;
    unsigned int ydest = 0;

    used_active = false;

    while (ybase < y0 + oversampled_height) {
        const unsigned int blocky = ybase / IFluidSim::block_size;
        const unsigned int celly = ybase % IFluidSim::block_size;

        const unsigned int copy_height = std::min(IFluidSim::block_size - celly, (height - ydest)*oversample);

        unsigned int xbase = x0;
        unsigned int xdest = 0;

        while (xbase < x0 + oversampled_width) {
            const unsigned int blockx = xbase / IFluidSim::block_size;
            const unsigned int cellx = xbase % IFluidSim::block_size;

            const unsigned int copy_width = std::min(IFluidSim::block_size - cellx, (width - xdest)*oversample);

            const unsigned int row_stride = dest_width - (copy_width+oversample-1)/oversample;

            /*std::cout << "xbase: " << xbase << "; "
                      << "ybase: " << ybase << "; "
                      << "cellx: " << cellx << "; "
                      << "celly: " << celly << "; "
                      << "copy_width: " << copy_width << "; "
                      << "copy_height: " << copy_height << "; "
                      << std::endl;*/

            const FluidBlock &block = *m_blocks.block(blockx, blocky);
            used_active = used_active || block.front_meta().active;

            copy_from_block(&dest[ydest*dest_width+xdest],
                            block,
                            cellx, celly,
                            copy_width, copy_height,
                            row_stride,
                            oversample);

            xbase += copy_width;
            if (copy_width % oversample != 0) {
                xbase += (oversample - (copy_width % oversample));
            }

            xdest += (copy_width+oversample-1) / oversample;
        }

        ybase += copy_height;
        if (copy_height % oversample != 0) {
            ybase += (oversample - (copy_height % oversample));
        }

        ydest += (copy_height+oversample-1) / oversample;
    }
}

/*void Fluid::copy_block_edge(Vector4f *dest,
                            int x0,
                            int y0,
                            unsigned int width,
                            unsigned int height,
                            const unsigned int oversample,
                            const unsigned int dest_width) const
{
    unsigned int xn_edge = 0;
    unsigned int xp_edge = 0;
    unsigned int yn_edge = 0;
    unsigned int yp_edge = 0;

    if (x0 < 0) {
        xn_edge = std::abs(x0);
        x0 += xn_edge * oversample;
        width -= xn_edge;
    }
    if (x0 + width >= m_blocks.cells_per_axis()) {
        xp_edge = std::abs(m_blocks.cells_per_axis() - (x0 + width));
        width -= xp_edge;
    }

    if (y0 < 0) {
        xn_edge = std::abs(y0);
        x0 += yn_edge * oversample;
        height -= yn_edge;
    }
    if (y0 + height >= m_blocks.cells_per_axis()) {
        yp_edge = std::abs(m_blocks.cells_per_axis() - (y0 + width));
        height -= yp_edge;
    }

    // copy what we have
    copy_block(&dest[yn_edge*dest_width+xn_edge], x0, y0, width, height, oversample, dest_width);

    // now fill the edges
    if (xn_edge) {
        for (unsigned int y = 0; y < height; y++) {
            Vector4f value = dest[(y+yn_edge)*dest_width+xn_edge];
            Vector4f *local_dest = &dest[(y+yn_edge)*dest_width];
            for (unsigned int x = 0; x < xn_edge; ++x) {
                *local_dest++ = value;
            }
        }
    }

    if (xp_edge) {
        for (unsigned int y = 0; y < height; y++) {
            Vector4f value = dest[(y+yn_edge)*dest_width+dest_width-xp_edge];
            Vector4f *local_dest = &dest[(y+yn_edge)*dest_width+dest_width-xp_edge+1];
            for (unsigned int x = 0; x < xp_edge; ++x) {
                *local_dest++ = value;
            }
        }
    }

    if (yn_edge) {
        for (unsigned int y = 0; y < height; y++) {
            memcpy(&dest[y*dest_width+xn_edge],
                   &dest[(y+yn_edge)*dest_width+xn_edge],
                   sizeof(Vector4f)*(dest_width-xn_edge-xp_edge));
        }
    }

    if (yp_edge) {
        for (unsigned int y = 0; y < height; y++) {
            memcpy(&dest[(y+height+yn_edge)*dest_width+xn_edge],
                   &dest[(y+height+yn_edge-1)*dest_width+xn_edge],
                   sizeof(Vector4f)*(dest_width-xn_edge-xp_edge));
        }
    }

}*/


}
