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
#include "engine/render/fancyterraindata.hpp"

#include "engine/math/algo.hpp"
#include "engine/math/intersect.hpp"

#include <iostream>

// #define TIMELOG_HITTEST

#ifdef TIMELOG_HITTEST
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;
#endif

namespace engine {

static io::Logger &logger = io::logging().get_logger("render.fancyterraindata");


FancyTerrainInterface::FancyTerrainInterface(sim::Terrain &terrain,
                                             const unsigned int grid_size):
    m_grid_size(grid_size),
    m_terrain(terrain),
    m_terrain_nt(terrain),
    m_terrain_nt_conn(terrain.terrain_updated().connect(
                          sigc::mem_fun(m_terrain_nt,
                                        &sim::NTMapGenerator::notify_update)
                          ))
{
    m_any_updated_conns.emplace_back(
                m_terrain_nt.field_updated().connect(
                    sigc::mem_fun(*this,
                                  &FancyTerrainInterface::any_updated)));
    m_any_updated_conns.emplace_back(
                m_terrain.terrain_updated().connect(
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

#ifdef DISABLE_QUADTREE
    logger.log(io::LOG_WARNING, "QuadTree hittest disabled at compile time!");
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

void FancyTerrainInterface::any_updated(const sim::TerrainRect&)
{
    logger.log(io::LOG_INFO, "Terrain LOD updated");
    m_field_updated.emit();
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
        const sim::Terrain::HeightField *heightfield = nullptr;
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
        const sim::Terrain::HeightField &field)
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

        /* std::cout << x << " " << y << std::endl; */

        if (x < 0 || y < 0) {
            continue;
        }

        const Vector3f p0(x, y, field[y*size+x]);
        const Vector3f p1(x, y+1, field[(y+1)*size+x]);
        const Vector3f p2(x+1, y+1, field[(y+1)*size+x+1]);
        const Vector3f p3(x+1, y, field[y*size+x+1]);

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
