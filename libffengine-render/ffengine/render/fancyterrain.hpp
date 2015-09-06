/**********************************************************************
File name: fancyterrain.hpp
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
#ifndef SCC_ENGINE_RENDER_FANCYTERRAIN_H
#define SCC_ENGINE_RENDER_FANCYTERRAIN_H

#include <atomic>
#include <unordered_map>
#include <unordered_set>

#include "ffengine/gl/resource.hpp"

#include "ffengine/render/scenegraph.hpp"
#include "ffengine/render/fancyterraindata.hpp"
#include "ffengine/render/fullterrain.hpp"

namespace ffe {

/**
 * Scenegraph node which renders a terrain using the CDLOD algorithm by
 * Strugar.
 */
class FancyTerrainNode: public FullTerrainRenderer
{
public:
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
    FancyTerrainNode(const unsigned int terrain_size,
                     const unsigned int grid_size,
                     FancyTerrainInterface &terrain,
                     GLResourceManager &resources);
    ~FancyTerrainNode() override;

private:
    GLResourceManager &m_resources;
    spp::EvaluationContext m_eval_context;

    FancyTerrainInterface &m_terrain_interface;

    const sim::Terrain &m_terrain;
    NTMapGenerator &m_terrain_nt;

    sigc::connection m_invalidate_cache_conn;

    bool m_linear_filter;

    Texture2D m_heightmap;
    Texture2D m_normalt;

    VBO m_vbo;
    IBO m_ibo;

    Material m_material;
    Material m_normal_debug_material;

    VBOAllocation m_vbo_allocation;
    IBOAllocation m_ibo_allocation;

    std::mutex m_cache_invalidation_mutex;
    sim::TerrainRect m_cache_invalidation;

    std::unordered_map<Material*, OverlayConfig> m_overlays;
    std::vector<RenderOverlay> m_render_overlays;

protected:
    void render_all(RenderContext &context, Material &material,
                    const FullTerrainNode::Slices &slices_to_render);

public:
    void attach_blend_texture(Texture2D *tex);
    void attach_grass_texture(Texture2D *tex);
    void attach_rock_texture(Texture2D *tex);

    /**
     * Mark the GPU side texture cache as invalid.
     *
     * The textures will be re-transferred on the next sync().
     *
     * @param part The part of the terrain which was changed. This is used to
     * optimize the amount of data which needs to be re-transferred.
     */
    void invalidate_cache(sim::TerrainRect part);

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

    bool enable_linear_filter() const
    {
        return m_linear_filter;
    }

    inline Material &material()
    {
        return m_material;
    }

    /**
     * Remove a previously registered overlay.
     *
     * @param mat The Material used for the overlay.
     *
     * @see configure_overlay
     */
    void remove_overlay(Material &mat);

    /**
     * Enable or disable linear filtering of the vertex data of the terrain.
     * Disabling the linear filter might considerably improve terrain rendering
     * performance on old systems, but will cause rendering bugs.
     *
     * The filter is enabled by default.
     *
     * @param filter Whether to enable linear filtering.
     */
    inline void set_enable_linear_filter(bool filter)
    {
        m_linear_filter = filter;
    }

public:
    void render(RenderContext &context,
                const FullTerrainNode &render_terrain) override;
    void sync(RenderContext &context,
              const FullTerrainNode &render_terrain) override;

};

}

#endif
