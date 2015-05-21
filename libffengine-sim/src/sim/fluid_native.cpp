/**********************************************************************
File name: fluid_native.cpp
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
#include "ffengine/sim/fluid_native.hpp"

#include <iostream>

#include "ffengine/math/algo.hpp"

// #define TIMELOG_FLUIDSIM

#ifdef TIMELOG_FLUIDSIM
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;

#define TIMELOG_ms(x) std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(x).count()
#endif

namespace sim {

static io::Logger &logger = io::logging().get_logger("sim.fluid.native");

static const FluidCell null_cell;

template <typename T>
T first(T v1, T v2)
{
    if (v1) {
        return v1;
    }
    return v2;
}

static unsigned int determine_worker_count()
{
    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) {
        thread_count = 2;
        logger.logf(io::LOG_ERROR,
                    "failed to determine hardware concurrency. "
                    "giving it a try with %u",
                    thread_count);
    }
    return thread_count;
}

/* sim::NativeFluidSim */

NativeFluidSim::NativeFluidSim(FluidBlocks &blocks,
                               const Terrain &terrain):
    m_blocks(blocks),
    m_terrain(terrain),
    m_worker_count(determine_worker_count()),
    m_run(false),
    m_done(false),
    m_worker_to_start(0),
    m_worker_terminate(false),
    m_worker_stopped(m_worker_count),
    m_worker_block_ctr(0),
    m_terminated(false),
    m_coordinator_thread(std::bind(&NativeFluidSim::coordinator_impl,
                                   this))
{
    if (!std::atomic_is_lock_free(&m_worker_block_ctr)) {
        logger.logf(io::LOG_WARNING, "fluid sim counter is not lock-free.");
    } else {
        logger.logf(io::LOG_INFO, "fluid sim counter is lock-free.");
    }

    m_worker_threads.reserve(m_worker_count);
    for (unsigned int i = 0; i < m_worker_count; i++) {
        m_worker_threads.emplace_back(std::bind(&NativeFluidSim::worker_impl, this));
    }
}

NativeFluidSim::~NativeFluidSim()
{
    m_terminated = true;
    m_control_wakeup.notify_all();
    m_coordinator_thread.join();
    for (auto &thread: m_worker_threads) {
        thread.join();
    }
}

void NativeFluidSim::coordinator_impl()
{
    logger.logf(io::LOG_INFO, "fluidsim: %u cells in %u blocks",
                m_blocks.cells_per_axis()*m_blocks.cells_per_axis(),
                m_blocks.blocks_per_axis()*m_blocks.blocks_per_axis());
    while (!m_terminated) {
        {
            std::unique_lock<std::mutex> control_lock(m_control_mutex);
            while (!m_run && !m_terminated) {
                m_control_wakeup.wait(control_lock);
            }
            if (m_terminated) {
                control_lock.unlock();
                std::lock_guard<std::mutex> done_lock(m_done_mutex);
                m_done = true;
                break;
            }
            m_run = false;
        }

#ifdef TIMELOG_FLUIDSIM
        const timelog_clock::time_point t0 = timelog_clock::now();
        timelog_clock::time_point t_sync, t_sim;
#endif
        // sync terrain
        TerrainRect updated_rect;
        {
            std::lock_guard<std::mutex> lock(m_terrain_update_mutex);
            updated_rect = m_terrain_update;
            m_terrain_update = NotARect;
        }
        if (!updated_rect.empty()) {
            logger.logf(io::LOG_INFO, "terrain to sync (%u vertices)",
                        updated_rect.area());
            sync_terrain(updated_rect);
        }

#ifdef TIMELOG_FLUIDSIM
        t_sync = timelog_clock::now();
#endif
        coordinator_run_workers();

        {
            std::lock_guard<std::mutex> done_lock(m_done_mutex);
            assert(!m_done);
            m_done = true;
        }
        m_done_wakeup.notify_all();

#ifdef TIMELOG_FLUIDSIM
        t_sim = timelog_clock::now();
        logger.logf(io::LOG_DEBUG, "fluid: sync time: %.2f ms",
                    TIMELOG_ms(t_sync - t0));
        logger.logf(io::LOG_DEBUG, "fluid: sim time: %.2f ms",
                    TIMELOG_ms(t_sim - t_sync));
#endif
    }
    {
        std::lock_guard<std::mutex> lock(m_worker_task_mutex);
        m_worker_terminate = true;
    }
    m_worker_wakeup.notify_all();
}

void NativeFluidSim::coordinator_run_workers()
{
    {
        std::lock_guard<std::mutex> lock(m_worker_done_mutex);
        assert(m_worker_stopped == m_worker_count);
        m_worker_stopped = 0;
    }
    {
        std::lock_guard<std::mutex> lock(m_worker_task_mutex);
        assert(m_worker_to_start == 0);
        m_worker_to_start = m_worker_count;
        // make sure all blocks run, we don’t need memory ordering, the mutex
        // implicitly orders
        m_worker_block_ctr.store(0, std::memory_order_relaxed);
    }
    // start all workers
    m_worker_wakeup.notify_all();

    // wait for all workers to finish
    {
        std::unique_lock<std::mutex> lock(m_worker_done_mutex);
        while (m_worker_stopped < m_worker_count) {
            m_worker_done_wakeup.wait(lock);
        }
        assert(m_worker_stopped == m_worker_count);
    }
    // some assertions
    assert(m_worker_block_ctr.load(std::memory_order_relaxed) >=
           m_blocks.blocks_per_axis()*m_blocks.blocks_per_axis());
    assert(m_worker_to_start == 0);
}

void NativeFluidSim::sync_terrain(TerrainRect rect)
{
    if (rect.x1() == m_terrain.size()) {
        rect.set_x1(m_terrain.size()-1);
    }
    if (rect.y1() == m_terrain.size()) {
        rect.set_y1(m_terrain.size()-1);
    }

    const unsigned int terrain_size = m_terrain.size();
    const Terrain::HeightField *field = nullptr;
    auto lock = m_terrain.readonly_field(field);
    for (unsigned int y = rect.y0(); y < rect.y1(); y++) {
        for (unsigned int x = rect.x0(); x < rect.x1(); x++) {
            FluidCellMeta *meta_ptr = m_blocks.cell_meta(x, y);
            const Terrain::height_t hsum =
                    (*field)[y*terrain_size+x]+
                    (*field)[y*terrain_size+x+1]+
                    (*field)[(y+1)*terrain_size+x]+
                    (*field)[(y+1)*terrain_size+x+1];
            meta_ptr->terrain_height = hsum / 4.f;
            m_blocks.block_for_cell(x, y)->set_active(true);
        }
    }
}

template <unsigned int dir, int flow_sign>
static inline FluidFloat flow(
        FluidCell &back,
        const FluidCell &front,
        const FluidCellMeta &meta,
        const FluidCell &neigh_front,
        const FluidCellMeta &neigh_meta,
        const FluidCell &flow_source)
{
    const FluidFloat dheight = front.fluid_height - neigh_front.fluid_height;
    const FluidFloat dterrain_height = meta.terrain_height - neigh_meta.terrain_height;
    const FluidFloat height_flow = (dheight+dterrain_height) * IFluidSim::flow_friction;

    const FluidFloat flow = (
            flow_sign*flow_source.fluid_flow[dir] * IFluidSim::flow_damping +
            height_flow * (FluidFloat(1.0) - IFluidSim::flow_damping));

    assert(std::abs(flow) < 1e10);
    assert(!isnan(flow) && !isinf(flow));

    FluidFloat applicable_flow =
            clamp(flow,
                  -neigh_front.fluid_height / FluidFloat(4.),
                  front.fluid_height / FluidFloat(4.));

    if (applicable_flow > FluidFloat(0)) {
        // flow is outgoing, check that neighbour height is appropriate
        if (front.fluid_height + meta.terrain_height < neigh_meta.terrain_height) {
            // we can’t go up there
            return applicable_flow;
        }
    } else if (applicable_flow < FluidFloat(0)) {
        // flow is incoming, check that our height is appropriate
        if (meta.terrain_height > neigh_front.fluid_height + neigh_meta.terrain_height) {
            // it can’t go up here
            return applicable_flow;
        }
    }

    // minimum height for fluid
    if (neigh_front.fluid_height < 1e-6 && applicable_flow < 1e-4) {
        return applicable_flow;
    } else if (front.fluid_height < 1e-6 && applicable_flow > -1e-4) {
        return applicable_flow;
    }
    back.fluid_height -= applicable_flow;

    return applicable_flow;
}

template <unsigned int dir>
static inline void full_flow(
        FluidCell &back,
        const FluidCell &front,
        const FluidCellMeta &meta,
        const FluidCell &left_front,
        const FluidCellMeta *left_meta,
        const FluidCell &right_front,
        const FluidCellMeta *right_meta)
{
    if (left_meta) {
        flow<dir, -1>(back, front, meta, left_front, *left_meta, left_front);
    }
    if (right_meta) {
        back.fluid_flow[dir] = flow<dir, 1>(back, front, meta, right_front, *right_meta, front);
        /*if (left_meta) {
            // we can smooth the flow a bit
            back.fluid_flow[dir] *= 128;
            back.fluid_flow[dir] += left_front.fluid_flow[dir] + right_front.fluid_flow[dir];
            back.fluid_flow[dir] /= 130;
        }*/
    }
    if (back.fluid_height < 0) {
        back.fluid_height = 0.f;
    }
}

void NativeFluidSim::update_active_block(FluidBlock &block)
{
    const unsigned int cy0 = block.y()*IFluidSim::block_size;
    const unsigned int cy1 = (block.y()+1)*IFluidSim::block_size;
    const unsigned int cx0 = block.x()*IFluidSim::block_size;
    const unsigned int cx1 = (block.x()+1)*IFluidSim::block_size;

    std::array<const FluidCell*, 8> neigh;
    std::array<const FluidCellMeta*, 8> neigh_meta;

    float change_accum = 0.f;
    float wet_cells = 0.f;

    FluidCell *back = block.local_cell_back(0, 0);
    const FluidCell *front = block.local_cell_front(0, 0);
    const FluidCellMeta *meta = block.local_cell_meta(0, 0);
    for (unsigned int cy = cy0; cy < cy1; cy++)
    {
        for (unsigned int cx = cx0; cx < cx1; cx++)
        {
            m_blocks.cell_front_neighbourhood(cx, cy, neigh, neigh_meta);

            back->fluid_height = front->fluid_height;

            {
                const FluidCell *left = first(neigh[Left], &null_cell);
                const FluidCell *right = first(neigh[Right], &null_cell);
                const FluidCellMeta *left_meta = neigh_meta[Left];
                const FluidCellMeta *right_meta = neigh_meta[Right];
                full_flow<0>(
                            *back, *front,
                            *meta,
                            *left, left_meta,
                            *right, right_meta);
            }
            {
                const FluidCell *left = first(neigh[Top], &null_cell);
                const FluidCell *right = first(neigh[Bottom], &null_cell);
                const FluidCellMeta *left_meta = neigh_meta[Top];
                const FluidCellMeta *right_meta = neigh_meta[Bottom];
                full_flow<1>(
                            *back, *front,
                            *meta,
                            *left, left_meta,
                            *right, right_meta);
            }

            if (meta->source_capacity > 0) {
                const float source_fluid_height = meta->source_height - meta->terrain_height;
                const float source_flow = clamp(
                            source_fluid_height - back->fluid_height,
                            -meta->source_capacity,
                            meta->source_capacity);
                back->fluid_height += source_flow;
                if (back->fluid_height < 0) {
                    back->fluid_height = 0;
                }
            }

            change_accum += std::abs(back->fluid_height - front->fluid_height);
            if (back->fluid_height > 0.f || front->fluid_height > 0.f) {
                wet_cells += 1.f;
            }

            ++back;
            ++front;
            ++meta;
        }
    }

    if (wet_cells > 0.f) {
        change_accum /= wet_cells;
    }
    block.accum_change(change_accum);

    FluidFloat change_plus_neighbours = block.back_meta().change;

    if (block.x() > 0) {
        FluidBlock &neighbour = *m_blocks.block(block.x()-1, block.y());
        if (neighbour.front_meta().active) {
            change_plus_neighbours += neighbour.front_meta().change * FluidBlock::CHANGE_TRANSFER_FACTOR;
        }
    }
    if (block.y() > 0) {
        FluidBlock &neighbour = *m_blocks.block(block.x(), block.y()-1);
        if (neighbour.front_meta().active) {
            change_plus_neighbours += neighbour.front_meta().change * FluidBlock::CHANGE_TRANSFER_FACTOR;
        }
    }
    if (block.x() < m_blocks.blocks_per_axis()-1) {
        FluidBlock &neighbour = *m_blocks.block(block.x()+1, block.y());
        if (neighbour.front_meta().active) {
            change_plus_neighbours += neighbour.front_meta().change * FluidBlock::CHANGE_TRANSFER_FACTOR;
        }
    }
    if (block.y() < m_blocks.blocks_per_axis()-1) {
        FluidBlock &neighbour = *m_blocks.block(block.x(), block.y()+1);
        if (neighbour.front_meta().active) {
            change_plus_neighbours += neighbour.front_meta().change * FluidBlock::CHANGE_TRANSFER_FACTOR;
        }
    }

    if (change_plus_neighbours < FluidBlock::CHANGE_BACKLOG_THRESHOLD) {
        logger.logf(io::LOG_DEBUG, "disabling block %u,%u after change of %.4f",
                    block.x(), block.y(),
                    block.back_meta().change);
        block.set_active(false);
    }
}

template <int dir, int flow_sign>
FluidFloat check_active_seams(FluidCell *local_seam_back,
                              const FluidCell *local_seam_front,
                              const FluidCellMeta *local_seam_meta,
                              const FluidCell *neighbour_seam_front,
                              const FluidCellMeta *neighbour_seam_meta)
{
    const FluidCell *flow_source_front = (
                flow_sign > 0
                ? local_seam_front
                : neighbour_seam_front);
    // when we’re going along the Y axis (flow direction 0) we have to use
    // the long stride, otherwise the cells are adjacent
    const unsigned int stride = (dir == 0 ? IFluidSim::block_size : 1);

    FluidFloat difference_accum = FluidFloat(0);
    FluidFloat wet_cells = FluidFloat(0);

    for (unsigned int i = 0; i < IFluidSim::block_size; i++)
    {
        flow<dir, flow_sign>(
                    *local_seam_back,
                    *local_seam_front,
                    *local_seam_meta,
                    *neighbour_seam_front,
                    *neighbour_seam_meta,
                    *flow_source_front);

        if (local_seam_front->fluid_height < 1e-4 && local_seam_back->fluid_height > 1e-5) {
            // activate immediately, this is new fluid entering our area
            difference_accum += 100.f;
        } else if (local_seam_back->fluid_height < 0) {
            // we have to activate immediately!
            // FIXME: use the proper threshold here
            difference_accum += 100.f;
            local_seam_back->fluid_height = 0.f;
        } else {
            const FluidFloat local_difference =
                    std::abs(local_seam_back->fluid_height - local_seam_front->fluid_height)
                    / local_seam_front->fluid_height;
            if (!isnan(local_difference)) {
                // nan can happen if nothing changed and cell is empty
                difference_accum += local_difference;
            }
        }

        if (local_seam_back->fluid_height > 0.f ||
                local_seam_front->fluid_height > 0.f ||
                neighbour_seam_front->fluid_height > 0.f)
        {
            wet_cells += 1;
        }

        local_seam_back += stride;
        local_seam_front += stride;
        local_seam_meta += stride;
        neighbour_seam_front += stride;
        neighbour_seam_meta += stride;
        flow_source_front += stride;
    }

    if (wet_cells > 0) {
        difference_accum /= wet_cells;
    }
    return difference_accum;
}

void NativeFluidSim::update_inactive_block(FluidBlock &block)
{
    // check the seams of the block for changes which are non-steady state
    // if the changes become too large we have to re-activate the block

    FluidFloat difference_accum = 0.f;
    bool any = false;

    if (block.x() > 0) {
        FluidBlock &neighbour = *m_blocks.block(block.x()-1, block.y());
        if (neighbour.front_meta().active) {
            any = true;
            difference_accum += check_active_seams<0, -1>(
                        block.local_cell_back(0, 0),
                        block.local_cell_front(0, 0),
                        block.local_cell_meta(0, 0),
                        neighbour.local_cell_front(IFluidSim::block_size-1, 0),
                        neighbour.local_cell_meta(IFluidSim::block_size-1, 0)
                        );
        }
    }
    if (block.y() > 0) {
        FluidBlock &neighbour = *m_blocks.block(block.x(), block.y()-1);
        if (neighbour.front_meta().active) {
            any = true;
            difference_accum += check_active_seams<1, -1>(
                        block.local_cell_back(0, 0),
                        block.local_cell_front(0, 0),
                        block.local_cell_meta(0, 0),
                        neighbour.local_cell_front(0, IFluidSim::block_size-1),
                        neighbour.local_cell_meta(0, IFluidSim::block_size-1)
                        );
        }
    }
    if (block.x() < m_blocks.blocks_per_axis()-1) {
        FluidBlock &neighbour = *m_blocks.block(block.x()+1, block.y());
        if (neighbour.front_meta().active) {
            any = true;
            difference_accum += check_active_seams<0, 1>(
                        block.local_cell_back(IFluidSim::block_size-1, 0),
                        block.local_cell_front(IFluidSim::block_size-1, 0),
                        block.local_cell_meta(IFluidSim::block_size-1, 0),
                        neighbour.local_cell_front(0, 0),
                        neighbour.local_cell_meta(0, 0)
                        );
        }
    }
    if (block.y() < m_blocks.blocks_per_axis()-1) {
        FluidBlock &neighbour = *m_blocks.block(block.x(), block.y()+1);
        if (neighbour.front_meta().active) {
            any = true;
            difference_accum += check_active_seams<1, -1>(
                        block.local_cell_back(0, IFluidSim::block_size-1),
                        block.local_cell_front(0, IFluidSim::block_size-1),
                        block.local_cell_meta(0, IFluidSim::block_size-1),
                        neighbour.local_cell_front(0, 0),
                        neighbour.local_cell_meta(0, 0)
                        );
        }
    }

    /*if (any) {
        logger.logf(io::LOG_WARNING, "%d %d %.4f",
                    block.x(), block.y(),
                    difference_accum);
    }*/

    if (difference_accum > FluidBlock::REACTIVATION_THRESHOLD)
    {
        logger.logf(io::LOG_DEBUG,
                    "reenabled block %u,%u with difference of %.4f",
                    block.x(), block.y(),
                    difference_accum);
        block.set_active(true);
    }
}

void NativeFluidSim::worker_impl()
{
    const unsigned int out_of_tasks_limit = m_blocks.blocks_per_axis()*m_blocks.blocks_per_axis();

    std::unique_lock<std::mutex> wakeup_lock(m_worker_task_mutex);
    while (!m_worker_terminate)
    {
        while (m_worker_to_start == 0 &&
               !m_worker_terminate)
        {
            m_worker_wakeup.wait(wakeup_lock);
        }
        if (m_worker_terminate) {
            return;
        }
        --m_worker_to_start;
        wakeup_lock.unlock();

        while (1) {
            const unsigned int my_block = m_worker_block_ctr.fetch_add(
                        1,
                        std::memory_order_relaxed);
            if (my_block >= out_of_tasks_limit)
            {
                break;
            }

            const unsigned int x = my_block % m_blocks.blocks_per_axis();
            const unsigned int y = my_block / m_blocks.blocks_per_axis();
            FluidBlock &block = *m_blocks.block(x, y);
            /*logger.logf(io::LOG_DEBUG, "fluid: %p got %u %u, active = %d",
                        this, x, y, block.front_meta().active);*/
            if (block.front_meta().active) {
                update_active_block(block);
            } else {
                update_inactive_block(block);
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_worker_done_mutex);
            m_worker_stopped += 1;
        }
        m_worker_done_wakeup.notify_all();

        wakeup_lock.lock();
    }
}

void NativeFluidSim::start_frame()
{
    m_blocks.swap_active_blocks();
    {
        std::unique_lock<std::mutex> lock(m_control_mutex);
        assert(!m_run);
        m_run = true;
    }
    m_control_wakeup.notify_all();
}

void NativeFluidSim::terrain_update(TerrainRect r)
{
    std::lock_guard<std::mutex> lock(m_terrain_update_mutex);
    m_terrain_update = bounds(r, m_terrain_update);
}

void NativeFluidSim::wait_for_frame()
{
    {
        std::unique_lock<std::mutex> lock(m_done_mutex);
        while (!m_done) {
            m_done_wakeup.wait(lock);
        }
        m_done = false;
    }
}


}
