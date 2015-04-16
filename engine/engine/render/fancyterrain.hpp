#ifndef SCC_ENGINE_RENDER_FANCYTERRAIN_H
#define SCC_ENGINE_RENDER_FANCYTERRAIN_H

#include <unordered_map>

#include "engine/render/scenegraph.hpp"

#include "engine/render/fancyterraindata.hpp"

namespace engine {

struct HeightmapSliceMeta
{
    /**
     * World coordinate of the x origin of this slice.
     */
    unsigned int basex;

    /**
     * World coordinate of the y origin of this slice.
     */
    unsigned int basey;

    /**
     * Size of this slice in world coordinates.
     */
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

    struct OverlayConfig
    {
        sim::TerrainRect clip_rect;
    };

    struct RenderOverlay
    {
        Material *material;
        sim::TerrainRect clip_rect;
    };

public:
    /**
     * Construct a fancy terrain node.
     *
     * @param terrain_interface The nice interface to the terrain to render.
     * @param texture_cache_size The square root of the number of tiles which
     * will be cached on the GPU. This will create a square texture with
     * grid_size*texture_cache_size texels on each edge.
     *
     * @opengl
     */
    FancyTerrainNode(FancyTerrainInterface &terrain,
                     const unsigned int texture_cache_size);

private:
    static constexpr float lod_range_base = 127;

    FancyTerrainInterface &m_terrain_interface;

    const unsigned int m_grid_size;
    const unsigned int m_texture_cache_size;
    const unsigned int m_min_lod;
    const unsigned int m_max_depth;

    sim::Terrain &m_terrain;
    FancyTerrainInterface::HeightFieldLODifier &m_terrain_lods;
    sim::MinMaxMapGenerator &m_terrain_minmax;
    sim::NTMapGenerator &m_terrain_nt;
    FancyTerrainInterface::NTMapLODifier &m_terrain_nt_lods;

    Texture2D m_heightmap;
    Texture2D m_normalt;

    VBO m_vbo;
    IBO m_ibo;

    Material m_material;
    Material m_normal_debug_material;

    VBOAllocation m_vbo_allocation;
    IBOAllocation m_ibo_allocation;

    std::unique_ptr<VAO> m_vao;
    std::unique_ptr<VAO> m_nd_vao;

    std::unordered_map<HeightmapSliceMeta, SlotIndex> m_allocated_slices;
    std::vector<SlotIndex> m_unused_slots;
    std::vector<HeightmapSliceMeta> m_heightmap_slots;

    std::vector<HeightmapSliceMeta> m_tmp_slices;

    std::vector<std::tuple<HeightmapSliceMeta, unsigned int> > m_render_slices;

    std::unordered_map<Material*, OverlayConfig> m_overlays;
    std::vector<RenderOverlay> m_render_overlays;

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
    void render_all(RenderContext &context, VAO &vao, Material &material);

public:
    void attach_grass_texture(Texture2D *tex);

    /**
     * Register and/or configure an overlay for rendering. If an overlay with
     * the given material is already registered, the settings will be
     * overriden.
     *
     * The overlay is rendered by rendering the terrain blocks which intersect
     * the given \a clip_rect using the given Material \a mat. The given
     * \a zoffset is applied in the Vertex Shader. The \a zoffset is in
     * relative units which will be scaled based on the distance of the viewer
     * to the overlay vertex. ``1.0`` is generally a good value.
     *
     * The overlay itself is rendered without modifying the depth buffer; it is
     * considered to be part of the terrain, which has already written its
     * z values.
     *
     * @param mat Material to render the overlay; the pointer to the material
     * is used as a key internally. The material must be configured with
     * configure_overlay_material().
     * @param clip_rect A rectangle for clipping the overlay rendering.
     *
     * @see remove_overlay
     */
    void configure_overlay(Material &mat,
                           const sim::TerrainRect &clip_rect);

    /**
     * Configure a material for use in overlay rendering.
     *
     * A vertex shader used for terrain rendering will be attached to the
     * materials shader. Then the shader is linked and the vertex textures
     * which are used for terrain rendering get attached.
     *
     * The vertex shader provides a structure to the following shader stages:
     *
     *     out TerrainData {
     *         vec3 world;  // world coordinate
     *         vec2 tc0;  // general purpose texture coordinate
     *         vec3 normal;  // normal vector
     *     }
     *
     * The vertex shader also takes a uniform float, \a zoffset, which is
     * initialized as ``1.0f``. It cas be used to control the amount of
     * distance the overlay has from the terrain, for Z-Buffer purposes. The
     * value is scaled with the distance of the viewer from the camera, thus,
     * generally, ``1.0f`` is a safe value.
     *
     * The material can be used to create an overlay.
     *
     * @param mat A material object whose shader is ready to link except that
     * it does not contain a vertex shader yet.
     * @return true if shader compilation and linking was successful, false
     * otherwise.
     *
     * @opengl
     */
    bool configure_overlay_material(Material &mat);


    /**
     * Remove a previously registered overlay.
     *
     * @param mat The Material used for the overlay.
     *
     * @see configure_overlay
     */
    void remove_overlay(Material &mat);

public:
    void render(RenderContext &context) override;
    void sync(Scene &scene) override;

};

}

#endif
