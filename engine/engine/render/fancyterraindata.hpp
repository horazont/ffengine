#ifndef SCC_ENGINE_RENDER_FANCYTERRAINDATA_H
#define SCC_ENGINE_RENDER_FANCYTERRAINDATA_H

#include "engine/math/shapes.hpp"

#include "engine/sim/terrain.hpp"

#define DISABLE_QUADTREE

namespace engine {

/**
 * A helper class to provide data which is derived from the main heightmap in
 * near realtime.
 *
 * It makes sure that the data providers get notified about heightmap changes
 * and update as soon as possible.
 *
 * A main usecase is with FancyTerrainNode, which requires an instance of this
 * class to render the terrain.
 */
class FancyTerrainInterface
{
public:
    FancyTerrainInterface(sim::Terrain &terrain,
                          const unsigned int grid_size);
    ~FancyTerrainInterface();

private:
    const unsigned int m_grid_size;

    sim::Terrain &m_terrain;
    sim::NTMapGenerator m_terrain_nt;

    sigc::connection m_terrain_nt_conn;

    std::vector<sigc::connection> m_any_updated_conns;

    sigc::signal<void> m_field_updated;

protected:
    void any_updated(const sim::TerrainRect &at);

public:
    inline unsigned int size() const
    {
        return m_terrain.size();
    }

    inline unsigned int grid_size() const
    {
        return m_grid_size;
    }

    inline sim::Terrain &terrain()
    {
        return m_terrain;
    }

    inline sim::NTMapGenerator &ntmap()
    {
        return m_terrain_nt;
    }

    inline sigc::signal<void> &field_updated()
    {
        return m_field_updated;
    }

    std::tuple<Vector3f, Vector3f, bool> hittest_quadtree(
            const Ray &ray,
            const unsigned int lod);
    std::tuple<Vector3f, bool> hittest(const Ray &ray);

};

std::tuple<Vector3f, bool> isect_terrain_ray(
        const Ray &ray,
        const unsigned int size,
        const sim::Terrain::HeightField &field);

}

#endif
