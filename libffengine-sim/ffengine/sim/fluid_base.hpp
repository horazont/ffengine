/**********************************************************************
File name: fluid_base.hpp
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
#ifndef SCC_SIM_FLUID_BASE_H
#define SCC_SIM_FLUID_BASE_H

#include <shared_mutex>
#include <vector>

#include "ffengine/sim/terrain.hpp"

namespace sim {

typedef float FluidFloat;


class IFluidSim
{
public:
    /**
     * The flow friction is in units of 1 / Time Unit. It controls the
     * influence of the absolute height difference between two adjacent cells
     * on the flow between the cells.
     */
    static const FluidFloat flow_friction;

    /**
     * The damping is unitless.
     */
    static const FluidFloat flow_damping;

    /**
     * The threshold for cells to be considered to carry a visible amount of
     * fluid.
     */
    static const FluidFloat visualization_threshold;

    static const unsigned int block_size;

public:
    virtual ~IFluidSim();

public:
    /**
     * Start a fluid simulation frame.
     *
     * It is an error to call this function when a frame is currently running.
     * In debug builds, this will either throw an exception or assert() out.
     * In release builds, this will either throw an exception or result in
     * weird behaviour, like skipped or duplicated frames.
     *
     * This method is thread-safe, but not neccessarily reentrant.
     *
     * @see wait_for_frame
     */
    virtual void start_frame() = 0;

    /**
     * Notify the fluid simulation that a terrain rect has changed.
     *
     * Multiple calls to this function are cumulative, in the sense that at
     * least all the rectangles which with which this function was called will
     * be synchronized.
     *
     * Implementations may (dis-)optimize this by merging rectangles into their
     * bounding rectangle.
     *
     * Changes are queued until the next frame starts (which is after calls to
     * start_frame()).
     *
     * This method is thread-safe, but not neccessarily reentrant.
     *
     * @param r Rectangle which was updated.
     */
    virtual void terrain_update(TerrainRect r) = 0;

    /**
     * Change the ocean level.
     *
     * The change is qued until the next frame starts.
     *
     * This method is thread-safe, but not neccessarily reentrant.
     *
     * @param level
     */
    virtual void set_ocean_level(const FluidFloat level) = 0;

    /**
     * Wait until the previously started frame has completed.
     *
     * This method, or the destructor, must be called exactly once per
     * start_frame() call.
     *
     * This method is thread-safe, but not reentrant.
     */
    virtual void wait_for_frame() = 0;

};

struct FluidCellMeta
{
    FluidCellMeta();

    /**
     * Height of the terrain in the cell. This is the average terrain height,
     * based on the height of the four adjacent vertices.
     */
    FluidFloat terrain_height;

    /**
     * Absolute height of a source working in this cell.
     */
    FluidFloat source_height;

    /**
     * Strength (Height Units per Frame) the source can source. If no source
     * is present in this cell, this is set to 0.
     */
    FluidFloat source_capacity;

};


struct FluidCell
{
    FluidCell();

    /**
     * Height of the fluid *above* the terrain in the cell. Thus, a cell with
     * `terrain_height = 2` and `fluid_height = 1` would have an absolute
     * fluid height of 3.
     *
     * The fluid and terrain heights are in Length Units (LU).
     */
    FluidFloat fluid_height;

    /**
     * Fluid flow in the cell.
     *
     * The flow is in units of Length Unit / Time Unit.
     */
    FluidFloat fluid_flow[2];
};


enum FluidNeighbours {
    Top = 0,
    TopRight = 1,
    Right = 2,
    BottomRight = 3,
    Bottom = 4,
    BottomLeft = 5,
    Left = 6,
    TopLeft = 7
};


/**
 * Metadata of a fluid block. This is kept in a separate structure for
 * double-buffering.
 */
struct FluidBlockMeta
{
    FluidBlockMeta();

    /**
     * true if the block is currently actively simulated, false otherwise.
     */
    bool active;

    /**
     * An internally scaled accumulated value which reflects the change which
     * happened during the last frames.
     */
    float change;

    /**
     * Whether the fluid block is a flat plane (within a certain margin of
     * error, typically std::numeric_limits<float>::epsilon()*
     * flat_absolute_height).
     */
    bool flat;

    /**
     * The absolute height of the block, if it is a flat plane.
     */
    float flat_absolute_height;
};


class FluidBlock
{
public:
    static const FluidFloat CHANGE_BACKLOG_FILTER_CONSTANT;
    static const FluidFloat CHANGE_BACKLOG_THRESHOLD;
    static const FluidFloat REACTIVATION_THRESHOLD;
    static const FluidFloat CHANGE_TRANSFER_FACTOR;

public:
    FluidBlock(const unsigned int x,
               const unsigned int y);

private:
    const unsigned int m_x;
    const unsigned int m_y;

    std::unique_ptr<FluidBlockMeta> m_front_meta;
    std::unique_ptr<FluidBlockMeta> m_back_meta;

    std::vector<FluidCellMeta> m_meta_cells;
    std::vector<FluidCell> m_back_cells;
    std::vector<FluidCell> m_front_cells;

public:
    inline unsigned int x() const
    {
        return m_x;
    }

    inline unsigned int y() const
    {
        return m_y;
    }

    inline FluidCell *local_cell_back(const unsigned int x,
                                      const unsigned int y)
    {
        return &m_back_cells[y*IFluidSim::block_size+x];
    }

    inline FluidCell *local_cell_front(const unsigned int x,
                                       const unsigned int y)
    {
        return &m_front_cells[y*IFluidSim::block_size+x];
    }

    inline const FluidCell *local_cell_front(const unsigned int x,
                                             const unsigned int y) const
    {
        return &m_front_cells[y*IFluidSim::block_size+x];
    }

    inline FluidCellMeta *local_cell_meta(const unsigned int x,
                                       const unsigned int y)
    {
        return &m_meta_cells[y*IFluidSim::block_size+x];
    }

    inline const FluidCellMeta *local_cell_meta(const unsigned int x,
                                             const unsigned int y) const
    {
        return &m_meta_cells[y*IFluidSim::block_size+x];
    }

    inline const FluidBlockMeta &front_meta() const
    {
        return *m_front_meta;
    }

    inline FluidBlockMeta &front_meta()
    {
        return *m_front_meta;
    }

    inline FluidBlockMeta &back_meta()
    {
        return *m_back_meta;
    }

    inline void set_active(bool new_active)
    {
        if (new_active != m_front_meta->active && new_active)
        {
            m_back_meta->change = CHANGE_BACKLOG_THRESHOLD * 3.f;
        }
        m_back_meta->active = new_active;
    }

    inline void accum_change(FluidFloat change)
    {
        m_back_meta->change =
                m_front_meta->change * CHANGE_BACKLOG_FILTER_CONSTANT
                + change * (FluidFloat(1) - CHANGE_BACKLOG_FILTER_CONSTANT);
    }

    inline void swap_buffers()
    {
        m_back_cells.swap(m_front_cells);
        *m_front_meta = *m_back_meta;
    }

};


struct FluidBlocks
{
public:
    FluidBlocks(const unsigned int block_count_per_axis);

private:
    const unsigned int m_blocks_per_axis;
    const unsigned int m_cells_per_axis;
    std::vector<FluidBlock> m_blocks;

    mutable std::shared_timed_mutex m_frontbuffer_mutex;

public:
    inline unsigned int blocks_per_axis() const
    {
        return m_blocks_per_axis;
    }

    inline unsigned int cells_per_axis() const
    {
        return m_cells_per_axis;
    }

    inline FluidBlock *block(const unsigned int blockx,
                             const unsigned int blocky)
    {
        return &m_blocks[blocky*m_blocks_per_axis+blockx];
    }

    inline const FluidBlock *block(const unsigned int blockx,
                                   const unsigned int blocky) const
    {
        return &m_blocks[blocky*m_blocks_per_axis+blockx];
    }

    inline FluidBlock *block_for_cell(const unsigned int cellx,
                                      const unsigned int celly)
    {
        const unsigned int blockx = cellx / IFluidSim::block_size;
        const unsigned int blocky = celly / IFluidSim::block_size;
        return block(blockx, blocky);
    }

    inline FluidCell *cell_back(const unsigned int x, const unsigned int y)
    {
        const unsigned int blockx = x / IFluidSim::block_size;
        const unsigned int localx = x % IFluidSim::block_size;
        const unsigned int blocky = y / IFluidSim::block_size;
        const unsigned int localy = y % IFluidSim::block_size;

        return block(blockx, blocky)->local_cell_back(localx, localy);
    }

    inline FluidCell *cell_front(const unsigned int x, const unsigned int y)
    {
        const unsigned int blockx = x / IFluidSim::block_size;
        const unsigned int localx = x % IFluidSim::block_size;
        const unsigned int blocky = y / IFluidSim::block_size;
        const unsigned int localy = y % IFluidSim::block_size;

        return block(blockx, blocky)->local_cell_front(localx, localy);
    }

    inline const FluidCell *cell_front(const unsigned int x,
                                       const unsigned int y) const
    {
        const unsigned int blockx = x / IFluidSim::block_size;
        const unsigned int localx = x % IFluidSim::block_size;
        const unsigned int blocky = y / IFluidSim::block_size;
        const unsigned int localy = y % IFluidSim::block_size;

        return block(blockx, blocky)->local_cell_front(localx, localy);
    }

    inline FluidCellMeta *cell_meta(const unsigned int x, const unsigned int y)
    {
        const unsigned int blockx = x / IFluidSim::block_size;
        const unsigned int localx = x % IFluidSim::block_size;
        const unsigned int blocky = y / IFluidSim::block_size;
        const unsigned int localy = y % IFluidSim::block_size;

        return block(blockx, blocky)->local_cell_meta(localx, localy);
    }

    inline const FluidCellMeta *cell_meta(const unsigned int x,
                                          const unsigned int y) const
    {
        const unsigned int blockx = x / IFluidSim::block_size;
        const unsigned int localx = x % IFluidSim::block_size;
        const unsigned int blocky = y / IFluidSim::block_size;
        const unsigned int localy = y % IFluidSim::block_size;

        return block(blockx, blocky)->local_cell_meta(localx, localy);
    }

    inline void cell_front_neighbourhood(
            const unsigned int x, const unsigned int y,
            std::array<const FluidCell*, 8> &neighbourhood,
            std::array<const FluidCellMeta*, 8> &neighbourhood_meta)
    {
        if (x > 0) {
            neighbourhood[Left] = cell_front(x-1, y);
            neighbourhood_meta[Left] = cell_meta(x-1, y);
            if (y > 0) {
                neighbourhood[TopLeft] = cell_front(x-1, y-1);
                neighbourhood_meta[TopLeft] = cell_meta(x-1, y-1);
            } else {
                neighbourhood[TopLeft] = nullptr;
                neighbourhood_meta[TopLeft] = nullptr;
            }
            if (y < m_cells_per_axis-1) {
                neighbourhood[BottomLeft] = cell_front(x-1, y+1);
                neighbourhood_meta[BottomLeft] = cell_meta(x-1, y+1);
            } else {
                neighbourhood[BottomLeft] = nullptr;
                neighbourhood_meta[BottomLeft] = nullptr;
            }
        } else {
            neighbourhood[Left] = nullptr;
            neighbourhood_meta[Left] = nullptr;
            neighbourhood[TopLeft] = nullptr;
            neighbourhood_meta[TopLeft] = nullptr;
            neighbourhood[BottomLeft] = nullptr;
            neighbourhood_meta[BottomLeft] = nullptr;
        }

        if (x < m_cells_per_axis-1) {
            neighbourhood[Right] = cell_front(x+1, y);
            neighbourhood_meta[Right] = cell_meta(x+1, y);
            if (y > 0) {
                neighbourhood[TopRight] = cell_front(x+1, y-1);
                neighbourhood_meta[TopRight] = cell_meta(x+1, y-1);
            } else {
                neighbourhood[TopRight] = nullptr;
                neighbourhood_meta[TopRight] = nullptr;
            }
            if (y < m_cells_per_axis-1) {
                neighbourhood[BottomRight] = cell_front(x+1, y+1);
                neighbourhood_meta[BottomRight] = cell_meta(x+1, y+1);
            } else {
                neighbourhood[BottomRight] = nullptr;
                neighbourhood_meta[BottomRight] = nullptr;
            }
        } else {
            neighbourhood[Right] = nullptr;
            neighbourhood_meta[Right] = nullptr;
            neighbourhood[TopRight] = nullptr;
            neighbourhood_meta[TopRight] = nullptr;
            neighbourhood[BottomRight] = nullptr;
            neighbourhood_meta[BottomRight] = nullptr;
        }

        if (y > 0) {
            neighbourhood[Top] = cell_front(x, y-1);
            neighbourhood_meta[Top] = cell_meta(x, y-1);
        } else {
            neighbourhood[Top] = nullptr;
            neighbourhood_meta[Top] = nullptr;
        }

        if (y < m_cells_per_axis-1) {
            neighbourhood[Bottom] = cell_front(x, y+1);
            neighbourhood_meta[Bottom] = cell_meta(x, y+1);
        } else {
            neighbourhood[Bottom] = nullptr;
            neighbourhood_meta[Bottom] = nullptr;
        }
    }

    inline void swap_active_blocks()
    {
        // we need to hold the frontbuffer lock to be safe -- this will ensure
        // that no user who is accessing the frontbuffer will suddenly be using
        // the backbuffer
        std::unique_lock<std::shared_timed_mutex> lock(m_frontbuffer_mutex);
        for (FluidBlock &block: m_blocks)
        {
            if (block.back_meta().active || block.front_meta().active)
            {
                block.swap_buffers();
            }
        }
    }

    inline std::shared_lock<std::shared_timed_mutex> read_frontbuffer() const
    {
        return std::shared_lock<std::shared_timed_mutex>(m_frontbuffer_mutex);
    }
};


}

#endif
