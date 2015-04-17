#include "engine/render/fancyterraindata.hpp"

#include "engine/math/algo.hpp"
#include "engine/math/intersect.hpp"

#include <iostream>

// #define TIMELOG_HITTEST
#define DISABLE_QUADTREE

#ifdef TIMELOG_HITTEST
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;
#endif

namespace engine {

static io::Logger &logger = io::logging().get_logger("render.fancyterraindata");


FancyTerrainInterface::FancyTerrainInterface(sim::Terrain &terrain,
                                             const unsigned int grid_size):
    m_grid_size(grid_size),
    m_minmax_lod_offset(log2_of_pot(m_grid_size-1)),
    m_terrain(terrain),
    m_terrain_lods(terrain),
    m_terrain_minmax(terrain),
    m_terrain_nt(terrain),
    m_terrain_nt_lods(m_terrain_nt),
    m_terrain_lods_conn(terrain.terrain_updated().connect(
                            sigc::mem_fun(m_terrain_lods,
                                          &HeightFieldLODifier::notify_update)
                            )),
    m_terrain_minmax_conn(terrain.terrain_updated().connect(
                              sigc::mem_fun(m_terrain_minmax,
                                            &sim::MinMaxMapGenerator::notify_update)
                              )),
    m_terrain_nt_conn(terrain.terrain_updated().connect(
                          sigc::mem_fun(m_terrain_nt,
                                        &sim::NTMapGenerator::notify_update)
                          )),
    m_terrain_nt_lods_conn(m_terrain_nt.field_updated().connect(
                               sigc::mem_fun(m_terrain_nt_lods,
                                             &NTMapLODifier::notify_update)
                               ))
{
    sim::TerrainRect full_terrain(0, 0,
                                  m_terrain.size(),
                                  m_terrain.size());

    if (!is_power_of_two(grid_size-1)) {
        throw std::runtime_error("grid_size must be power-of-two plus one");
    }
    if (((terrain.size()-1) / (m_grid_size-1))*(m_grid_size-1) != terrain.size()-1)
    {
        throw std::runtime_error("grid_size-1 must divide terrain size-1 evenly");
    }

#ifdef DISABLE_QUADTREE
    logger.log(io::LOG_WARNING, "QuadTree hittest disabled at compile time!");
#endif
}

FancyTerrainInterface::~FancyTerrainInterface()
{
    m_terrain_nt_lods_conn.disconnect();
    m_terrain_nt_conn.disconnect();
    m_terrain_minmax_conn.disconnect();
    m_terrain_lods_conn.disconnect();
}

std::tuple<Vector3f, Vector3f, bool> FancyTerrainInterface::hittest_quadtree(
        const Ray &ray,
        const unsigned int lod_index)
{
    const sim::MinMaxMapGenerator::MinMaxFieldLODs *lods = nullptr;
    auto lock = m_terrain_minmax.readonly_lods(lods);
    if (lods->size() <= lod_index) {
        return std::make_tuple(Vector3f(), Vector3f(), false);
    }
    return isect_terrain_quadtree_ray(ray, m_terrain.size()-1, *lods);
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
        const sim::MinMaxMapGenerator::MinMaxFieldLODs *lods = nullptr;
        auto mm_lock = m_terrain_minmax.readonly_lods(lods);
        const sim::Terrain::HeightField *heightfield = nullptr;
        auto height_lock = m_terrain.readonly_field(heightfield);
#ifdef TIMELOG_HITTEST
        t_lock = timelog_clock::now();
        result =
#else
        return
#endif
        isect_terrain_ray(ray, m_terrain.size(), *heightfield, *lods);
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


std::tuple<float, float, bool> isect_terrain_quadtree_ray_recurse(
        const Ray &ray,
        const unsigned int quad_size,
        const unsigned int xlod,
        const unsigned int ylod,
        const unsigned int lod_index,
        const sim::MinMaxMapGenerator::MinMaxFieldLODs &lods)
{
    const unsigned int lod_size = 1 << ((lods.size()-1) - lod_index);
    const unsigned int x0 = xlod * quad_size;
    const unsigned int y0 = ylod * quad_size;
    const sim::MinMaxMapGenerator::MinMaxField &field = lods[lod_index];
#ifdef DISABLE_QUADTREE
    sim::Terrain::height_t hmin = sim::Terrain::min_height;
    sim::Terrain::height_t hmax = sim::Terrain::max_height;
#else
    sim::Terrain::height_t hmin, hmax;
    std::tie(hmin, hmax) = field[ylod*lod_size+xlod];
#endif

    /* std::cout << xlod << " " << ylod << " " << lod_size << " " << quad_size << " " << hmin << " " << hmax << std::endl; */

    const AABB box{Vector3f(x0,
                            y0,
                            hmin),
                   Vector3f(x0 + quad_size,
                            y0 + quad_size,
                            hmax)};

    float tmin, tmax;
    if (!isect_aabb_ray(box, ray, tmin, tmax)) {
        /* std::cout << "AABB test failed" << std::endl; */
        return std::make_tuple(tmin, tmax, false);
    }
#ifdef DISABLE_QUADTREE
    if (true) {
#else
    if (lod_index == 4) {
#endif
        /* std::cout << "Max LOD reached " << tmin << " " << tmax << std::endl; */
        return std::make_tuple(std::max(tmin, 0.f), tmax, true);
    }

    tmin = std::numeric_limits<float>::max();
    tmax = std::numeric_limits<float>::min();
    float test_min, test_max;
    bool any_hit = false, hit;
    // top left
    std::tie(test_min, test_max, hit) = isect_terrain_quadtree_ray_recurse(
                ray,
                quad_size / 2,
                xlod*2,
                ylod*2,
                lod_index-1,
                lods);
    if (hit) {
        tmin = std::min(tmin, test_min);
        tmax = std::max(tmax, test_max);
        any_hit = true;
    }

    // bottom left
    std::tie(test_min, test_max, hit) = isect_terrain_quadtree_ray_recurse(
                ray,
                quad_size / 2,
                xlod*2,
                ylod*2+1,
                lod_index-1,
                lods);
    if (hit) {
        tmin = std::min(tmin, test_min);
        tmax = std::max(tmax, test_max);
        any_hit = true;
    }

    // bottom right
    std::tie(test_min, test_max, hit) = isect_terrain_quadtree_ray_recurse(
                ray,
                quad_size / 2,
                xlod*2+1,
                ylod*2+1,
                lod_index-1,
                lods);
    if (hit) {
        tmin = std::min(tmin, test_min);
        tmax = std::max(tmax, test_max);
        any_hit = true;
    }

    // top right
    std::tie(test_min, test_max, hit) = isect_terrain_quadtree_ray_recurse(
                ray,
                quad_size / 2,
                xlod*2+1,
                ylod*2,
                lod_index-1,
                lods);
    if (hit) {
        tmin = std::min(tmin, test_min);
        tmax = std::max(tmax, test_max);
        any_hit = true;
    }

    return std::make_tuple(tmin, tmax, any_hit);
}


std::tuple<Vector3f, Vector3f, bool> isect_terrain_quadtree_ray(
        const Ray &ray,
        const unsigned int size,
        const sim::MinMaxMapGenerator::MinMaxFieldLODs &lods)
{
    float tmin, tmax;
    bool hit;
    if (lods.size() < log2_of_pot(size-1)+1)
    {
        logger.log(io::LOG_WARNING, "not enough LOD data for quadtree tracing");
        hit = isect_aabb_ray(AABB{Vector3f(0, 0, 0), Vector3f(size, size, 1024)},
                             ray, tmin, tmax);
        return std::make_tuple(ray.origin+ray.direction*tmin,
                               ray.origin+ray.direction*tmax,
                               hit);
    }
    std::tie(tmin, tmax, hit) = isect_terrain_quadtree_ray_recurse(
                ray,
                size,
                0,
                0,
                lods.size()-1,
                lods);

    if (!hit) {
        return std::make_tuple(Vector3f(), Vector3f(), false);
    }

    return std::make_tuple(ray.origin+ray.direction*tmin,
                           ray.origin+ray.direction*tmax,
                           true);
}


std::tuple<Vector3f, bool> isect_terrain_ray(
        const Ray &ray,
        const unsigned int size,
        const sim::Terrain::HeightField &field,
        const sim::MinMaxMapGenerator::MinMaxFieldLODs &lods)
{
#ifdef TIMELOG_HITTEST
    timelog_clock::time_point t0;
    timelog_clock::time_point t_quad;
    timelog_clock::time_point t_final;
    std::tuple<Vector3f, bool> result;
#endif
    typedef RasterIterator<float> FieldIterator;

    Vector3f min, max;
    bool hit = false;
#ifdef TIMELOG_HITTEST
    t0 = timelog_clock::now();
#endif
    std::tie(min, max, hit) = isect_terrain_quadtree_ray(ray, size-1, lods);
#ifdef TIMELOG_HITTEST
    t_quad = timelog_clock::now();
#endif
    if (!hit) {
        return std::make_tuple(Vector3f(), hit);
    }

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
