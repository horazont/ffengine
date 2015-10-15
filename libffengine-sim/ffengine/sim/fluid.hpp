/**********************************************************************
File name: fluid.hpp
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
#ifndef SCC_SIM_FLUID_H
#define SCC_SIM_FLUID_H

#include <sig11/sig11.hpp>

#include "ffengine/sim/terrain.hpp"

#include "ffengine/common/utils.hpp"

#include "ffengine/sim/objects.hpp"
#include "ffengine/sim/fluid_base.hpp"


namespace sim {

/**
 * Fluid simulation.
 *
 * The fluid simulation is a huge fun project.
 */
class Fluid
{
public:
    /**
     * Fluid source/sink.
     *
     * The simulation sets the fluid to the given absolute height, so it may also
     * act as a sink if placed correctly.
     */
    class Source: public Object
    {
    public:
        Source(Object::ID object_id,
               const Vector2f pos,
               const float radius,
               const float absolute_height,
               const float capacity);
        Source(Object::ID object_id,
               const float x, const float y,
               const float radius,
               const float absolute_height,
               const float capacity);

    public:
        /**
         * Origin (center) of the fluid source.
         */
        Vector2f m_pos;

        /**
         * Radius of the fluid source.
         */
        float m_radius;

        /**
         * Absolute height of the fluid at the source.
         */
        float m_absolute_height;

        /**
         * The height of fluid per cell the source may source or the sink may
         * sink, in Height Units.
         */
        float m_capacity;

    };

public:
    Fluid(const Terrain &terrain);
    ~Fluid();

private:
    FluidBlocks m_blocks;
    std::vector<Source*> m_sources;
    std::unique_ptr<IFluidSim> m_impl;
    float m_ocean_level;

    bool m_sources_invalidated;

    /* owned by Fluid */
    sigc::connection m_terrain_update_conn;

private:
    void copy_from_block(Vector4f *dest,
                         const sim::FluidBlock &src,
                         const unsigned int x0,
                         const unsigned int y0,
                         const unsigned int width,
                         const unsigned int height,
                         const unsigned int row_stride,
                         const unsigned int step) const;

protected:
    void map_source(Source *obj);
    TerrainRect source_rect(Source *obj) const;
    void terrain_updated(TerrainRect r);
    void validate_sources();

public:
    inline FluidBlocks &blocks()
    {
        return m_blocks;
    }

    inline const FluidBlocks &blocks() const
    {
        return m_blocks;
    }

    void start();
    void to_gl_texture() const;
    void wait_for();

public:
    /**
     * @name Source management
     * Manage fluid sources.
     */

    /**@{*/

    /**
     * Add a fluid source to the simulation.
     *
     * This calls invalidate_sources().
     *
     * Adding the same source multiple times may lead to interesting and
     * inefficient behaviour, but is not checked against.
     *
     * This method is not thread-safe. It may be called while the simulation
     * is running, as it does not conflict with simulation data, but not
     * concurrently with start().
     *
     * @param obj Source to add
     */
    void add_source(Source *obj);

    /**
     * Invalidate the mapping of sources to the cell metadata.
     *
     * This causes the cell metadata to be re-written for all sources before
     * the next simulation frame. Be aware that this will not remove stale
     * information, this must be done by calling unmap_source() before
     * changing source parameters.
     *
     * This method is not thread-safe. It may be called while the simulation
     * is running, as it does not conflict with simulation data, but not
     * concurrently with start().
     */
    void invalidate_sources();

    /**
     * Remove a fluid source from the simulation. This calls unmap_source() on
     * the source.
     *
     * This method is not thread-safe and must not be called while the
     * simulation is running or concurrently with start().
     *
     * @param obj Source object to remove.
     */
    void remove_source(Source *obj);

    /**
     * Unmap a source from the cell metadata. This will erase all source
     * information in the cells which are affected by the given source.
     *
     * This also calls invalidate_sources(), as it does not re-write the
     * information of other sources which possibly intersect the unmapped
     * source.
     *
     * It is not checked that the source actually belongs to this
     * simulation :).
     *
     * This method is not thread-safe and must not be called while the
     * simulation is running or concurrently with start().
     *
     * @param obj Source to unmap.
     */
    void unmap_source(Source *obj);

    inline const std::vector<Source*> sources() const
    {
        return m_sources;
    }

    /**@}*/

    /**
      * @name Ocean level
      */
    /**@{*/

    /**
     * Return the current ocean level in absolute height units.
     */
    inline float ocean_level() const
    {
        return m_ocean_level;
    }

    /**
     * Set the ocean level to the given level.
     *
     * @param level New ocean level in absolute height units.
     */
    void set_ocean_level(const float level);

    /**@}*/

    /**
     * Reset the whole simulation.
     *
     * This also initialises the cells with the ocean level.
     *
     * This method is not thread-safe and must not be called while the
     * simulation is running or concurrently with start().
     */
    void reset();

public:
    /**
     * @name Extracting data rects
     */

    /**@{*/

    /**
     * Copy a block of render fluid data into the given buffer.
     *
     * The data is formatted as four-dimensional vectors, with the components
     * being, from x to w: terrain height, fluid height, flow x, flow y.
     *
     * This function must not be called concurrently with start(), but it is
     * safe to use after start has returned.
     *
     * @param dest Pointer to the first element to write.
     * @param x0 The start x coordinate
     * @param y0 The start y coordinate
     * @param width The width of the block in the x direction, in world space.
     * @param height The height of the block in the y direction, in world space.
     * @param oversample How many cells to skip in each direction on each step.
     * This is useful to create a NEAREST-type LOD map. With width and height
     * staying the same, this does not affect the number of cells copied, but
     * the size of the rect covered.
     * @param dest_width The width of the destination buffer. This is used to
     * calculate the stride between two consecutive rows when incrementing
     * \a dest.
     */
    void copy_block(Vector4f *dest,
                    const unsigned int x0,
                    const unsigned int y0,
                    const unsigned int width,
                    const unsigned int height,
                    const unsigned int oversample,
                    const unsigned int dest_width,
                    bool &used_active) const;

    /**
     * Copy a block of render fluid data into the given buffer. If the rect
     * selected for copying is larger than the fluid space, the edge data is
     * repeated. Corners are created by interpolating between the edge data.
     *
     * @param dest Pointer to the first element to write.
     * @param x0 The start x coordinate; may be negative
     * @param y0 The start y coordinate; may be negative
     * @param width The number of cells copied in the x direction
     * @param height The number of cells copied in the y direction
     * @param oversample How many cells to skip in each direction on each step.
     * This is useful to create a NEAREST-type LOD map. With width and height
     * staying the same, this does not affect the number of cells copied, but
     * the size of the rect covered.
     * @param dest_width The width of the destination buffer. This is used to
     * calculate the stride between two consecutive rows when incrementing
     * \a dest.
     */
    /*void copy_block_edge(Vector4f *dest,
                         int x0,
                         int y0,
                         unsigned int width,
                         unsigned int height,
                         const unsigned int oversample,
                         const unsigned int dest_width) const;*/

    /**@}*/

};


}

#endif
