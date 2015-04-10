#ifndef SCC_ENGINE_RENDER_FANCYTERRAIN_H
#define SCC_ENGINE_RENDER_FANCYTERRAIN_H

#include <unordered_map>

#include "engine/render/scenegraph.hpp"

#include "engine/sim/terrain.hpp"

namespace engine {

struct HeightmapSliceMeta
{
    unsigned int basex;
    unsigned int basey;
    unsigned int lod;

    bool operator<(const HeightmapSliceMeta &other) const;

};

}

namespace std {

template<>
struct hash<engine::HeightmapSliceMeta>
{
    typedef engine::HeightmapSliceMeta argument_type;
    typedef typename std::hash<unsigned int>::result_type result_type;

    hash():
        m_uint_hash()
    {

    }

private:
    hash<unsigned int> m_uint_hash;

public:
    result_type operator()(const argument_type &value) const
    {
        return m_uint_hash(value.basex)
                ^ m_uint_hash(value.basey)
                ^ m_uint_hash(value.lod);
    }

};

}

namespace engine {

/**
 * Scenegraph node which renders a terrain using the CDLOD algorithm by
 * Strugar.
 *
 * It is mainly controlled by the ``grid_size`` and ``texture_cache_size``
 * parameters, as passed to the constructor.
 *
 * The *grid_size* controlls the number of vertices in a single grid tile used
 * for terrain rendering. For the smallest tile, this is equivalent to the
 * number of heightmap points covered. This is thus the world size of the
 * smallest (i.e. most precise, like with mipmaps) level-of-detail.
 *
 * For rendering, parts of the heightmap (and belonging normal maps etc.) are
 * cached on the GPU, depending on which are needed. The size of the cache is
 * controlled by *texture_cache_size*, which is the number of **tiles** along
 * one axis of the cache texture. For a grid size of 64 and a texture cache
 * size of 32, a square texture of size 2048 (along each edge) will be
 * created.
 */
class FancyTerrainNode: public scenegraph::Node
{
public:
    typedef unsigned int SlotIndex;
    typedef sim::FieldLODifier<sim::Terrain::height_t, sim::Terrain> HeightFieldLODifier;
    typedef sim::FieldLODifier<sim::NTMapGenerator::element_t, sim::NTMapGenerator> NTMapLODifier;

public:
    /**
     * Construct a fancy terrain node.
     *
     * @param terrain The terrain to render.
     * @param grid_size The length of an edge of the smallest square tile
     * to render. Must divide the size of the terrain with a power of two.
     * @param texture_cache_size The square root of the number of tiles which
     * will be cached on the GPU. This will create a square texture with
     * grid_size*texture_cache_size texels on each edge.
     */
    FancyTerrainNode(sim::Terrain &terrain,
                     const unsigned int grid_size,
                     const unsigned int texture_cache_size);
    ~FancyTerrainNode();

private:
    static constexpr float lod_range_base = 127;

    const unsigned int m_grid_size;
    const unsigned int m_texture_cache_size;
    const unsigned int m_min_lod;
    const unsigned int m_max_depth;

    sim::Terrain &m_terrain;
    HeightFieldLODifier m_terrain_lods;
    sim::MinMaxMapGenerator m_terrain_minmax;
    sim::NTMapGenerator m_terrain_nt;
    NTMapLODifier m_terrain_nt_lods;
    sigc::connection m_terrain_lods_conn;
    sigc::connection m_terrain_minmax_conn;
    sigc::connection m_terrain_nt_conn;
    sigc::connection m_terrain_nt_lods_conn;

    Texture2D m_heightmap;
    Texture2D m_normalt;

    VBO m_vbo;
    IBO m_ibo;

    Material m_material;

    VBOAllocation m_vbo_allocation;
    IBOAllocation m_ibo_allocation;

    std::unique_ptr<VAO> m_vao;

    std::unordered_map<HeightmapSliceMeta, SlotIndex> m_allocated_slices;
    std::vector<SlotIndex> m_unused_slots;
    std::vector<HeightmapSliceMeta> m_heightmap_slots;

    std::vector<HeightmapSliceMeta> m_tmp_slices;

    std::vector<std::tuple<HeightmapSliceMeta, unsigned int> > m_render_slices;
    Vector3f m_render_viewpoint;

protected:
    void collect_slices_recurse(
            std::vector<HeightmapSliceMeta> &requested_slices,
            const unsigned int depth,
            const unsigned int relative_x,
            const unsigned int relative_y,
            const Vector3f &viewpoint,
            const sim::MinMaxMapGenerator::MinMaxFieldLODs &minmaxfields);
    void collect_slices(
            std::vector<HeightmapSliceMeta> &requested_slices,
            const Vector3f &viewpoint);
    void compute_heightmap_lod(unsigned int basex,
                               unsigned int basey,
                               unsigned int lod);
    void render_all(RenderContext &context);

public:
    void render(RenderContext &context) override;
    void sync() override;

};

}

#endif
