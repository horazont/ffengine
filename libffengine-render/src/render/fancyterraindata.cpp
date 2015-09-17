/**********************************************************************
File name: fancyterraindata.cpp
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
#include "ffengine/render/fancyterraindata.hpp"

#include "ffengine/math/algo.hpp"
#include "ffengine/math/intersect.hpp"

#include <cstring>
#include <iostream>

// #define TIMELOG_HITTEST

#ifdef TIMELOG_HITTEST
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;
#endif

namespace ffe {

static io::Logger &logger = io::logging().get_logger("render.fancyterraindata");


NTMapGenerator::NTMapGenerator(const sim::Terrain &source):
    m_source(source)
{
    start();
}

NTMapGenerator::~NTMapGenerator()
{
    tear_down();
}

void NTMapGenerator::worker_impl(const sim::TerrainRect &updated)
{
    const unsigned int source_size = m_source.size();

    sim::TerrainRect to_update = updated;
    if (to_update.x0() > 0) {
        to_update.set_x0(to_update.x0() - 1);
    }
    if (to_update.y0() > 0) {
        to_update.set_y0(to_update.y0() - 1);
    }
    if (to_update.x1() < source_size) {
        to_update.set_x1(to_update.x1() + 1);
    }
    if (to_update.y1() < source_size) {
        to_update.set_y1(to_update.y1() + 1);
    }

    const unsigned int dst_width = (to_update.x1() - to_update.x0());
    const unsigned int dst_height = (to_update.y1() - to_update.y0());
    const unsigned int src_xoffset = (to_update.x0() == 0 ? 0 : 1);
    const unsigned int src_yoffset = (to_update.y0() == 0 ? 0 : 1);
    const unsigned int src_x0 = to_update.x0() - src_xoffset;
    const unsigned int src_y0 = to_update.y0() - src_yoffset;
    const unsigned int src_width = dst_width
            + (to_update.x0() == 0 ? 0 : 1)
            + (to_update.x1() == source_size ? 0 : 1);
    const unsigned int src_height = dst_height
            + (to_update.y0() == 0 ? 0 : 1)
            + (to_update.y1() == source_size ? 0 : 1);


    sim::Terrain::Field source_latch(src_width*src_height);
    // this worker really takes a long time, we will copy the source
    {
        const sim::Terrain::Field *heightmap = nullptr;
        auto source_lock = m_source.readonly_field(heightmap);
        for (unsigned int ysrc = src_y0, ylatch = 0;
             ylatch < src_height;
             ylatch++, ysrc++)
        {
            memcpy(&source_latch[ylatch*src_width],
                    &(*heightmap)[ysrc*source_size+src_x0],
                    sizeof(sim::Terrain::Field::value_type)*src_width);
        }

    }

    NTField dest(dst_width*dst_height);

    bool has_ym = to_update.y0() > 0;
    for (unsigned int y = 0;
         y < dst_height;
         y++)
    {
        bool has_xm = to_update.x0() > 0;
        bool has_yp = to_update.y1() < source_size || y < dst_height - 1;
        for (unsigned int x = 0;
             x < dst_width;
             x++)
        {
            const bool has_xp = to_update.x1() < source_size || x < dst_width - 1;
            Vector3f normal(0, 0, 0);
            float tangent_eZ = 0;


            const sim::Terrain::height_t y0x0 = source_latch[(y+src_yoffset)*src_width+x+src_xoffset][sim::Terrain::HEIGHT_ATTR];
            sim::Terrain::height_t ymx0;
            sim::Terrain::height_t ypx0;
            sim::Terrain::height_t y0xm;
            sim::Terrain::height_t y0xp;

            Vector3f tangent_x1;
            Vector3f tangent_x2;

            Vector3f tangent_y1;
            Vector3f tangent_y2;

            if (has_ym)
            {
                ymx0 = source_latch[(y+src_yoffset-1)*src_width+x+src_xoffset][sim::Terrain::HEIGHT_ATTR];
                tangent_y1 = Vector3f(0, 1, y0x0 - ymx0);
            }
            if (has_yp)
            {
                ypx0 = source_latch[(y+src_yoffset+1)*src_width+x+src_xoffset][sim::Terrain::HEIGHT_ATTR];
                tangent_y2 = Vector3f(0, 1, ypx0 - y0x0);
            }
            if (has_xm)
            {
                y0xm = source_latch[(y+src_yoffset)*src_width+x+src_xoffset-1][sim::Terrain::HEIGHT_ATTR];
                const float z = y0x0 - y0xm;
                tangent_x1 = Vector3f(1, 0, z);
                tangent_eZ += z;
            }
            if (has_xp)
            {
                y0xp = source_latch[(y+src_yoffset)*src_width+x+src_xoffset+1][sim::Terrain::HEIGHT_ATTR];
                const float z = y0xp - y0x0;
                tangent_x2 = Vector3f(1, 0, z);
                tangent_eZ += z;
                if (!has_xm) {
                    // correct for division by two later on
                    tangent_eZ *= 2;
                }
            } else {
                // correct for division by two later on
                tangent_eZ *= 2;
            }

            assert((has_xm || has_xp) && (has_ym || has_yp));

            if (has_xm && has_ym) {
                // above left face
                normal += tangent_x1 % tangent_y1;
            }
            if (has_xp && has_ym) {
                // above right face
                normal += tangent_x2 % tangent_y1;
            }

            if (has_xm && has_yp) {
                // below left face
                normal += tangent_x1 % tangent_y2;
            }
            if (has_xp && has_yp) {
                // below right face
                normal += tangent_x2 % tangent_y2;
            }

            /* if (!has_xm || !has_ym || !has_yp || !has_xp) {
                std::cerr << x << " " << y << std::endl;
            } */

            normal.normalize();

            dest[y*dst_width+x] = Vector4f(normal, tangent_eZ / 2.);

            has_xm = true;
        }
        has_ym = true;
    }

    /*std::cerr << dst_width*dst_height << std::endl;
    if (dst_width*dst_height < 1000) {
        for (unsigned int y = 0; y < dst_height; y++) {
            for (unsigned int x = 0; x < dst_width; x++) {
                std::cerr << "y = " << y << "; x = " << x << "; " << dest[y*dst_width+x] << std::endl;
            }
        }
    }*/

    {
        std::unique_lock<std::shared_timed_mutex> lock(m_data_mutex);
        m_field.resize(source_size*source_size);
        for (unsigned int ydst = 0, ystore = src_y0+src_yoffset;
             ydst < dst_height;
             ystore++, ydst++)
        {
            memcpy(&m_field[ystore*source_size+src_x0+src_xoffset],
                    &dest[ydst*dst_width],
                    sizeof(Vector4f)*dst_width);
        }
    }

    m_field_updated.emit(updated);
}

std::shared_lock<std::shared_timed_mutex> NTMapGenerator::readonly_field(
        const NTField *&field) const
{
    field = &m_field;
    return std::shared_lock<std::shared_timed_mutex>(m_data_mutex);
}


FancyTerrainInterface::FancyTerrainInterface(const sim::Terrain &terrain,
                                             const unsigned int grid_size):
    m_grid_size(grid_size),
    m_terrain(terrain),
    m_terrain_nt(terrain),
    m_terrain_nt_conn(terrain.heightmap_updated().connect(
                          sigc::mem_fun(m_terrain_nt,
                                        &NTMapGenerator::notify_update)
                          ))
{
    m_any_updated_conns.emplace_back(
                m_terrain_nt.field_updated().connect(
                    sigc::mem_fun(*this,
                                  &FancyTerrainInterface::any_updated)));
    m_any_updated_conns.emplace_back(
                m_terrain.heightmap_updated().connect(
                    sigc::mem_fun(*this,
                                  &FancyTerrainInterface::any_updated)));

    const unsigned int tiles = ((terrain.size()-1) / (m_grid_size-1));

    if (tiles*(m_grid_size-1) != terrain.size()-1)
    {
        throw std::runtime_error("grid_size-1 must divide terrain size-1 evenly");
    }
    if ((1u<<log2_of_pot(tiles)) != tiles)
    {
        throw std::runtime_error("(terrain size-1) / (grid size-1) must be power of two");
    }

#ifndef DISABLE_QUADTREE
    logger.log(io::LOG_WARNING, "QuadTree hittest (SLOW!) enabled at compile time!");
#endif
}

FancyTerrainInterface::~FancyTerrainInterface()
{
    for (auto &conn: m_any_updated_conns)
    {
        conn.disconnect();
    }
    m_any_updated_conns.clear();
    m_terrain_nt_conn.disconnect();
}

void FancyTerrainInterface::any_updated(const sim::TerrainRect &part)
{
    logger.log(io::LOG_INFO, "Terrain LOD updated");
    m_field_updated.emit(part);
}

std::tuple<Vector3f, bool> FancyTerrainInterface::hittest(const Ray &ray)
{
#ifdef TIMELOG_HITTEST
    timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_lock;
    timelog_clock::time_point t_done;
    std::tuple<Vector3f, bool> result;
#endif
    {
        const sim::Terrain::Field *heightfield = nullptr;
        auto height_lock = m_terrain.readonly_field(heightfield);
#ifdef TIMELOG_HITTEST
        t_lock = timelog_clock::now();
        result =
#else
        return
#endif
        isect_terrain_ray(ray, m_terrain.size(), *heightfield);
    }
#ifdef TIMELOG_HITTEST
    t_done = timelog_clock::now();
    logger.logf(io::LOG_DEBUG, "hittest: time to lock: %.2f ms",
                std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(t_lock - t0).count());
    logger.logf(io::LOG_DEBUG, "hittest: time from lock to hit: %.2f ms",
                std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(t_done - t_lock).count());
    return result;
#endif
}


std::tuple<Vector3f, bool> isect_terrain_ray(
        const Ray &ray,
        const unsigned int size,
        const sim::Terrain::Field &field)
{
#ifdef TIMELOG_HITTEST
    timelog_clock::time_point t0;
    timelog_clock::time_point t_quad;
    timelog_clock::time_point t_final;
    std::tuple<Vector3f, bool> result;
#endif
    typedef RasterIterator<float> FieldIterator;

    float tmin, tmax;
    bool hit = false;
#ifdef TIMELOG_HITTEST
    t0 = timelog_clock::now();
#endif

    hit = isect_aabb_ray(AABB{Vector3f(0, 0, sim::Terrain::min_height),
                              Vector3f(size, size, sim::Terrain::max_height)},
                         ray,
                         tmin, tmax);

#ifdef TIMELOG_HITTEST
    t_quad = timelog_clock::now();
#endif
    if (!hit || tmin < 0) {
        return std::make_tuple(Vector3f(), hit);
    }

    const Vector3f min = ray.origin + ray.direction*tmin;
    const Vector3f max = ray.origin + ray.direction*tmax;

    // min and max point to the enter and exit point of the range of terrain
    // fields we matched. we now have to march across this line to find the
    // field which actually hit.

    const float x0 = min[eX];
    const float y0 = min[eY];

    const float x1 = max[eX];
    const float y1 = max[eY];

    for (auto iter = FieldIterator(x0, y0, x1, y1);
         iter;
         ++iter)
    {
        int x, y;
        std::tie(x, y) = *iter;

        if (x < 0 || y < 0 || x >= (int)(size-1) || y >= (int)(size-1)) {
            continue;
        }

        const Vector3f p0(x, y, field[y*size+x][sim::Terrain::HEIGHT_ATTR]);
        const Vector3f p1(x, y+1, field[(y+1)*size+x][sim::Terrain::HEIGHT_ATTR]);
        const Vector3f p2(x+1, y+1, field[(y+1)*size+x+1][sim::Terrain::HEIGHT_ATTR]);
        const Vector3f p3(x+1, y, field[y*size+x+1][sim::Terrain::HEIGHT_ATTR]);

        float t;
        std::tie(t, hit) = isect_ray_triangle(ray, p0, p1, p2);
        if (hit) {
#ifdef TIMELOG_HITTEST
            result =
#else
            return
#endif
                    std::make_tuple(ray.origin + ray.direction*t,
                                    true);
#ifdef TIMELOG_HITTEST
            break;
#endif
        }
        std::tie(t, hit) = isect_ray_triangle(ray, p2, p0, p3);
        if (hit) {
#ifdef TIMELOG_HITTEST
            result =
#else
            return
#endif
                    std::make_tuple(ray.origin + ray.direction*t,
                                   true);
#ifdef TIMELOG_HITTEST
            break;
#endif
        }
        hit = false;
    }

#ifdef TIMELOG_HITTEST
    t_final = timelog_clock::now();
    logger.logf(io::LOG_DEBUG, "hittest: quadtree time: %.2f ms",
                std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > > (t_quad - t0).count());
    logger.logf(io::LOG_DEBUG, "hittest: raster time: %.2f ms",
                std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > > (t_final - t_quad).count());
    if (hit) {
        return result;
    }
#endif
    return std::make_tuple(min, false);
}


}
