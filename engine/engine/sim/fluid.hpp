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

#include "engine/sim/terrain.hpp"

#include "engine/common/utils.hpp"


namespace sim {


typedef float FluidFloat;

struct FluidCellMeta
{
    FluidCellMeta();

    /**
     * Height of the terrain in the cell. This is the average terrain height,
     * based on the height of the four adjacent vertices.
     */
    FluidFloat terrain_height;
};


struct FluidCell
{
    FluidCell();

    /**
     * Height of the fluid *above* the terrain in the cell. Thus, a cell with
     * `terrain_height = 2` and `fluid_height = 1` would have an absolute
     * fluid height of 3.
     */
    FluidFloat fluid_height;

    /**
     * Fluid flow in the cell.
     */
    FluidFloat fluid_flow[2];
};


/**
 * Fluid source/sink.
 *
 * The simulation sets the fluid to the given absolute height, so it may also
 * act as a sink if placed correctly.
 */
struct FluidSource
{
    /**
     * X origin (center) of the fluid source.
     */
    float x0;

    /**
     * Y origin (center) of the fluid source.
     */
    float y0;

    /**
     * Radius of the fluid source.
     */
    float radius;

    /**
     * Absolute height of the fluid at the source.
     */
    FluidFloat absolute_height;
};


/**
 * Metadata for a fluid engine block
 *
 * A block is a square group of cells. Each block can be active or inactive,
 * which describes whether it is fully simulated in a frame or not.
 */
struct FluidBlockMeta
{
    /**
     * Is the block active? If so, it is fully simulated in a frame. Blocks
     * become inactive if there has been no significant change in a frame.
     */
    bool active;
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


struct FluidBlocks
{
    FluidBlocks(const unsigned int block_count_per_axis,
                const unsigned int block_size);

    const unsigned int m_block_size;
    const unsigned int m_blocks_per_axis;
    const unsigned int m_cells_per_axis;
    std::vector<FluidBlockMeta> m_block_meta;
    std::vector<FluidCellMeta> m_meta_cells;
    std::vector<FluidCell> m_front_cells;
    std::vector<FluidCell> m_back_cells;

    mutable std::shared_timed_mutex m_frontbuffer_mutex;

public:
    inline FluidCell *cell_back(const unsigned int x, const unsigned int y)
    {
        return &m_back_cells[y*m_cells_per_axis+x];
    }

    inline FluidCell *cell_front(const unsigned int x, const unsigned int y)
    {
        return &m_front_cells[y*m_cells_per_axis+x];
    }

    inline const FluidCell *cell_front(const unsigned int x, const unsigned int y) const
    {
        return &m_front_cells[y*m_cells_per_axis+x];
    }

    inline FluidCellMeta *cell_meta(const unsigned int x, const unsigned int y)
    {
        return &m_meta_cells[y*m_cells_per_axis+x];
    }

    inline const FluidCellMeta *cell_meta(const unsigned int x, const unsigned int y) const
    {
        return &m_meta_cells[y*m_cells_per_axis+x];
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

    inline void cell_pair(const unsigned int x, const unsigned int y,
                   FluidCell *&front, FluidCell *&back)
    {
        front = cell_front(x, y);
        back = cell_back(x, y);
    }

    inline void swap_buffers()
    {
        // we need to hold the frontbuffer lock to be safe -- this will ensure
        // that no user who is accessing the frontbuffer will suddenly be using
        // the backbuffer
        std::unique_lock<std::shared_timed_mutex> lock(m_frontbuffer_mutex);
        m_back_cells.swap(m_front_cells);
    }
};


/**
 * Fluid simulation.
 *
 * The fluid simulation is a huge fun project.
 */
class Fluid
{
public:
    static const FluidFloat flow_friction;
    static const FluidFloat flow_damping;
    static const unsigned int block_size;

protected:
    enum class JobType {
        PREPARE = 0,
        UPDATE = 1
    };

public:
    Fluid(const Terrain &terrain);
    ~Fluid();

private:
    const Terrain &m_terrain;
    const unsigned int m_block_count;
    const unsigned int m_worker_count;
    FluidBlocks m_blocks;
    std::vector<FluidSource> m_sources;

    std::vector<std::future<void> > m_task_futures;

    /* guarded by m_terrain_update_mutex */
    std::mutex m_terrain_update_mutex;
    TerrainRect m_terrain_update;

    /* owned by Fluid */
    sigc::connection m_terrain_update_conn;

    /* guarded by m_worker_task_mutex */
    std::mutex m_worker_task_mutex;
    std::condition_variable m_worker_wakeup;
    JobType m_worker_job;
    unsigned int m_worker_to_start;
    bool m_worker_terminate;

    /* guarded by m_worker_done_mutex */
    std::mutex m_worker_done_mutex;
    std::condition_variable m_worker_done_wakeup;
    unsigned int m_worker_stopped;

    /* atomic */
    std::atomic<unsigned int> m_worker_block_ctr;
    std::atomic_bool m_terminated;

    std::thread m_coordinator_thread;

    /* owned by m_coordinator_thread */
    std::vector<std::thread> m_worker_threads;

    /* guarded by m_control_mutex */
    std::mutex m_control_mutex;
    std::condition_variable m_control_wakeup;
    bool m_run;

    /* guarded by m_done_mutex */
    std::mutex m_done_mutex;
    std::condition_variable m_done_wakeup;
    bool m_done;

protected:
    void coordinator_impl();
    void coordinator_run_workers(JobType job);

    void prepare_block(const unsigned int x, const unsigned int y);

    void sync_terrain(TerrainRect rect);

    void terrain_updated(TerrainRect r);

    void update_block(const unsigned int x, const unsigned int y);

    void worker_impl();

public:
    inline FluidBlocks &blocks()
    {
        return m_blocks;
    }

    void start();
    void to_gl_texture() const;
    void wait_for();

};

}

#endif
