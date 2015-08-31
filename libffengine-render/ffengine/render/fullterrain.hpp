/**********************************************************************
File name: fullterrain.hpp
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
#ifndef SCC_RENDER_FULLTERRAIN_H
#define SCC_RENDER_FULLTERRAIN_H

#include "ffengine/render/scenegraph.hpp"

namespace engine {

struct TerrainSlice
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

    /**
     * Is the slice actually valid?
     */
    bool valid;

    TerrainSlice();
    TerrainSlice(unsigned int basex, unsigned int basey, unsigned int lod);
    TerrainSlice(const TerrainSlice &ref) = default;
    TerrainSlice &operator=(const TerrainSlice &ref) = default;

    inline bool operator==(const TerrainSlice &other) const
    {
        if (!valid || !other.valid) {
            return (valid == other.valid);
        }
        return (basex == other.basex) && (basey == other.basey) && (lod == other.lod);
    }
};


class FullTerrainRenderer;


/**
 * The FullTerrainNode offers services for FullTerrainRender instances. It
 * calculates the visible slices as well as any LOD parameters and offers that
 * information to users during sync() and render().
 *
 * The LOD information is generated for use with the CDLOD algorithm by
 * Strugar and the general detail level can be controlled using
 * set_detail_level().
 *
 * New renderers can be added using emplace().
 */
class FullTerrainNode: public scenegraph::Node
{
public:
    typedef std::vector<TerrainSlice> Slices;

public:
    FullTerrainNode(const unsigned int terrain_size,
                    const unsigned int grid_size);

private:
    const unsigned int m_terrain_size;
    const unsigned int m_grid_size;
    const unsigned int m_max_depth;

    unsigned int m_detail_level;
    float m_lod_range_base;

    std::vector<std::unique_ptr<FullTerrainRenderer> > m_renderers;

    Slices m_render_slices;

private:
    /**
     * Generate TerrainSlice instances and fill the m_render_slices vector.
     *
     * @param invdepth The inverse of the LOD tree depth. Start with
     * m_max_depth for a full tree.
     * @param relative_x The current x position inside the tree.
     * @param relative_y The current y position inside the tree.
     * @param viewpoint The viewpoint to use for LOD calculations.
     * @param frustum The frustum to use for exclusion calculations.
     */
    void collect_slices_recurse(const unsigned int invdepth,
            const unsigned int relative_x,
            const unsigned int relative_y,
            const Vector3f &viewpoint,
            const std::array<Plane, 6> &frustum);

public:
    /**
     * Create and add a new renderer. Return a reference to the newly created
     * object. The FullTerrainNode holds ownership of the new object.
     *-
     * The arguments are passed using std::forward to the constructor of \a T.
     */
    template <typename T, typename... arg_ts>
    T &emplace(arg_ts&&... args)
    {
        auto obj = std::make_unique<T>(std::forward<arg_ts>(args)...);
        T &result = *obj;
        m_renderers.emplace_back(std::move(obj));
        return result;
    }

public:
    /**
     * Return the current detail level.
     *
     * The detail level is a number between 0 and max_detail_level(),
     * inclusively. The higher the detail level, the higher the rendered detail
     * is.
     *
     * @return Current detail level
     */
    inline unsigned int detail_level() const
    {
        return m_detail_level;
    }

    /**
     * The maximum detail level available.
     *
     * Setting the maximum detail level will produce **a lot** of geometry.
     */
    inline unsigned int max_detail_level() const
    {
        return m_max_depth;
    }

    /**
     * A factor used for LOD calculations in CDLOD shaders.
     */
    inline float scale_to_radius() const
    {
        return m_lod_range_base / (m_grid_size-1);
    }

    /**
     * Set a new detail level.
     *
     * The value is clamped to max_detail_level().
     *
     * @param level The new level to use.
     */
    void set_detail_level(unsigned int level);

    inline const Slices &slices_to_render() const
    {
        return m_render_slices;
    }

public:
    /**
     * Render all FullTerrainRenderer instances registered with the
     * TerrainSlice which were deemed visible during sync().
     *
     * @param context The render context to use.
     */
    void render(RenderContext &context) override;

    /**
     * Determine the set pieces of terrain which are visible and call sync()
     * on all FullTerrainRenderer instances.
     *
     * @param context The render context to use.
     */
    void sync(RenderContext &context) override;

};



class FullTerrainRenderer
{
public:
    virtual ~FullTerrainRenderer();

public:
    virtual void sync(RenderContext &context,
                      const FullTerrainNode &slices_to_render) = 0;
    virtual void render(RenderContext &context,
                        const FullTerrainNode &slices_to_render) = 0;

};

}

#endif
