#include "engine/render/fancyterraindata.hpp"

#include "engine/math/algo.hpp"
#include "engine/math/intersect.hpp"

#include <iostream>


namespace engine {

FancyTerrainInterface::FancyTerrainInterface(sim::Terrain &terrain,
                                             const unsigned int grid_size):
    m_grid_size(grid_size),
    m_minmax_lod_offset(log2_of_pot(m_grid_size-1)),
    m_terrain(terrain),
    m_terrain_lods(terrain),
    m_terrain_minmax(terrain),
    m_terrain_nt(terrain),
    m_terrain_nt_lods(m_terrain_nt),
    m_terrain_lods_conn(terrain.terrain_changed().connect(
                            std::bind(&HeightFieldLODifier::notify,
                                      &m_terrain_lods))),
    m_terrain_minmax_conn(terrain.terrain_changed().connect(
                              std::bind(&sim::MinMaxMapGenerator::notify,
                                        &m_terrain_minmax))),
    m_terrain_nt_conn(terrain.terrain_changed().connect(
                          std::bind(&sim::NTMapGenerator::notify,
                                    &m_terrain_nt))),
    m_terrain_nt_lods_conn(m_terrain_nt.field_changed().connect(
                               std::bind(&NTMapLODifier::notify,
                                         &m_terrain_nt_lods)))
{
    m_terrain_lods.notify();
    m_terrain_minmax.notify();
    m_terrain_nt.notify();

    if (!is_power_of_two(grid_size-1)) {
        throw std::runtime_error("grid_size must be power-of-two plus one");
    }
    if (((terrain.size()-1) / (m_grid_size-1))*(m_grid_size-1) != terrain.size()-1)
    {
        throw std::runtime_error("grid_size-1 must divide terrain size-1 evenly");
    }
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
    const sim::MinMaxMapGenerator::MinMaxFieldLODs *lods = nullptr;
    auto mm_lock = m_terrain_minmax.readonly_lods(lods);
    const sim::Terrain::HeightField *heightfield = nullptr;
    auto height_lock = m_terrain.readonly_field(heightfield);
    return isect_terrain_ray(ray, m_terrain.size(), *heightfield, *lods);
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
    sim::Terrain::height_t hmin, hmax;
    std::tie(hmin, hmax) = field[ylod*lod_size+xlod];

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
    if (lod_index == 0) {
        /* std::cout << "Max LOD reached " << tmin << " " << tmax << std::endl; */
        return std::make_tuple(tmin, tmax, true);
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
    typedef RasterIterator<float> FieldIterator;

    Vector3f min, max;
    bool hit = false;
    std::tie(min, max, hit) = isect_terrain_quadtree_ray(ray, size-1, lods);
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
        bool hit;
        std::tie(t, hit) = isect_ray_triangle(ray, p0, p1, p2);
        if (hit) {
            return std::make_tuple(ray.origin + ray.direction*t,
                                   true);
        }
        std::tie(t, hit) = isect_ray_triangle(ray, p3, p1, p2);
        if (hit) {
            return std::make_tuple(ray.origin + ray.direction*t,
                                   true);
        }
    }

    return std::make_tuple(min, false);
}


}
