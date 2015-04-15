#ifndef SCC_ENGINE_RENDER_FANCYTERRAINDATA_H
#define SCC_ENGINE_RENDER_FANCYTERRAINDATA_H

#include "engine/math/shapes.hpp"

#include "engine/sim/terrain.hpp"


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
    typedef sim::FieldLODifier<sim::Terrain::height_t, sim::Terrain> HeightFieldLODifier;
    typedef sim::FieldLODifier<sim::NTMapGenerator::element_t, sim::NTMapGenerator> NTMapLODifier;

public:
    FancyTerrainInterface(sim::Terrain &terrain,
                          const unsigned int grid_size);
    ~FancyTerrainInterface();

private:
    const unsigned int m_grid_size;
    const unsigned int m_minmax_lod_offset;

    sim::Terrain &m_terrain;
    HeightFieldLODifier m_terrain_lods;
    sim::MinMaxMapGenerator m_terrain_minmax;
    sim::NTMapGenerator m_terrain_nt;
    NTMapLODifier m_terrain_nt_lods;

    sigc::connection m_terrain_lods_conn;
    sigc::connection m_terrain_minmax_conn;
    sigc::connection m_terrain_nt_conn;
    sigc::connection m_terrain_nt_lods_conn;

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

    inline HeightFieldLODifier &heightmap_lods()
    {
        return m_terrain_lods;
    }

    inline sim::MinMaxMapGenerator &heightmap_minmax()
    {
        return m_terrain_minmax;
    }

    inline sim::NTMapGenerator &ntmap()
    {
        return m_terrain_nt;
    }

    inline NTMapLODifier &ntmap_lods()
    {
        return m_terrain_nt_lods;
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
        const sim::Terrain::HeightField &field,
        const sim::MinMaxMapGenerator::MinMaxFieldLODs &lods);

std::tuple<Vector3f, Vector3f, bool> isect_terrain_quadtree_ray(
        const Ray &ray,
        const unsigned int size,
        const sim::MinMaxMapGenerator::MinMaxFieldLODs &minmax_lods);

}

#endif
