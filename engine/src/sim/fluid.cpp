/**********************************************************************
File name: fluid.cpp
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
#include "engine/sim/fluid.hpp"

#include <GL/glew.h>

#include <cstring>
#include <iostream>

#include "engine/gl/util.hpp"
#include "engine/io/log.hpp"
#include "engine/math/algo.hpp"


#define TIMELOG_FLUIDSIM

#ifdef TIMELOG_FLUIDSIM
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;

#define TIMELOG_ms(x) std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(x).count()
#endif


namespace sim {

static io::Logger &logger = io::logging().get_logger("sim.fluid");
static const FluidCell null_cell;


unsigned int determine_worker_count()
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


template <typename T>
T first(T v1, T v2)
{
    if (v1) {
        return v1;
    }
    return v2;
}


const FluidFloat Fluid::flow_damping = 0.995;
/* const FluidFloat Fluid::flow_damping = 0.1; */
const FluidFloat Fluid::flow_friction = 0.6;
const unsigned int Fluid::block_size = 60;


FluidCellMeta::FluidCellMeta():
    terrain_height(0)
{

}


FluidCell::FluidCell():
    fluid_height(0.f),
    fluid_flow{0.f, 0.f}
{

}


FluidBlocks::FluidBlocks(const unsigned int block_count_per_axis,
                         const unsigned int block_size):
    m_block_size(block_size),
    m_blocks_per_axis(block_count_per_axis),
    m_cells_per_axis(m_block_size*m_blocks_per_axis),
    m_block_meta(m_blocks_per_axis*m_blocks_per_axis),
    m_meta_cells(m_cells_per_axis*m_cells_per_axis),
    m_front_cells(m_cells_per_axis*m_cells_per_axis),
    m_back_cells(m_cells_per_axis*m_cells_per_axis)
{

}


Fluid::Fluid(const Terrain &terrain):
    m_terrain(terrain),
    m_block_count((m_terrain.size()-1) / block_size),
    m_worker_count(determine_worker_count()),
    m_blocks(m_block_count, block_size),
    m_terrain_update_conn(m_terrain.terrain_updated().connect(
                              sigc::mem_fun(*this, &Fluid::terrain_updated))),
    m_worker_to_start(0),
    m_worker_terminate(false),
    m_worker_stopped(m_worker_count),
    // initializing the m_block_ctr to a value >= m_block_count**2 will make
    // all workers go to sleep immediately
    m_worker_block_ctr(0),
    m_terminated(false),
    m_coordinator_thread(std::bind(&Fluid::coordinator_impl, this)),
    m_run(false),
    m_done(false)
{
    if (m_block_count * block_size != m_terrain.size()-1) {
        throw std::runtime_error("Terrain size minus one must be a multiple of"
                                 " fluid block size, which is "+
                                 std::to_string(block_size));
    }

    if (!std::atomic_is_lock_free(&m_worker_block_ctr)) {
        logger.logf(io::LOG_WARNING, "fluid sim counter is not lock-free.");
    } else {
        logger.logf(io::LOG_INFO, "fluid sim counter is lock-free.");
    }

    m_worker_threads.reserve(m_worker_count);
    for (unsigned int i = 0; i < m_worker_count; i++) {
        m_worker_threads.emplace_back(std::bind(&Fluid::worker_impl, this));
    }
}

Fluid::~Fluid()
{
    m_terrain_update_conn.disconnect();
    m_terminated = true;
    m_control_wakeup.notify_all();
    m_coordinator_thread.join();
    for (auto &thread: m_worker_threads) {
        thread.join();
    }
}

void Fluid::coordinator_impl()
{
    const unsigned int worker_count = m_worker_count;

    logger.logf(io::LOG_INFO, "fluidsim: %u cells in %u blocks",
                m_blocks.m_cells_per_axis*m_blocks.m_cells_per_axis,
                m_blocks.m_blocks_per_axis*m_blocks.m_blocks_per_axis);
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
        timelog_clock::time_point t_sync, t_prepare, t_sim;
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

        /*coordinator_run_workers(JobType::PREPARE);*/

#ifdef TIMELOG_FLUIDSIM
        t_prepare = timelog_clock::now();
#endif
        coordinator_run_workers(JobType::UPDATE);

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
        logger.logf(io::LOG_DEBUG, "fluid: prep time: %.2f ms",
                    TIMELOG_ms(t_prepare - t_sync));
        logger.logf(io::LOG_DEBUG, "fluid: sim time: %.2f ms",
                    TIMELOG_ms(t_sim - t_prepare));
#endif
    }
    {
        std::lock_guard<std::mutex> lock(m_worker_task_mutex);
        m_worker_terminate = true;
    }
    m_worker_wakeup.notify_all();
}

void Fluid::coordinator_run_workers(JobType job)
{
    {
        std::lock_guard<std::mutex> lock(m_worker_done_mutex);
        assert(m_worker_stopped == m_worker_count);
        m_worker_stopped = 0;
    }
    {
        std::lock_guard<std::mutex> lock(m_worker_task_mutex);
        assert(m_worker_to_start == 0);
        // configure job
        m_worker_job = job;
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
           m_block_count*m_block_count);
    assert(m_worker_to_start == 0);
}

void Fluid::prepare_block(const unsigned int x, const unsigned int y)
{
    // XXX: this step is currently not used, JobType::PREPARE does not get
    // emitted by the controller!

    // copy the frontbuffer to the backbuffer
    const unsigned int cy0 = y*m_blocks.m_block_size;
    const unsigned int cy1 = (y+1)*m_blocks.m_block_size;
    const unsigned int cx0 = x*m_blocks.m_block_size;
    for (unsigned int cy = cy0; cy < cy1; cy++)
    {
        const FluidCell *front = m_blocks.cell_front(cx0, cy);
        FluidCell *back = m_blocks.cell_back(cx0, cy);
        memcpy(back, front, sizeof(FluidCell)*m_blocks.m_block_size);
    }
}

void Fluid::sync_terrain(TerrainRect rect)
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
        FluidCellMeta *meta_ptr = m_blocks.cell_meta(rect.x0(), y);
        for (unsigned int x = rect.x0(); x < rect.x1(); x++) {
            const Terrain::height_t hsum =
                    (*field)[y*terrain_size+x]+
                    (*field)[y*terrain_size+x+1]+
                    (*field)[(y+1)*terrain_size+x]+
                    (*field)[(y+1)*terrain_size+x+1];
            meta_ptr->terrain_height = hsum / 4.f;
            ++meta_ptr;
        }
    }
}

void Fluid::terrain_updated(TerrainRect r)
{
    std::unique_lock<std::mutex> lock(m_terrain_update_mutex);
    m_terrain_update = bounds(r, m_terrain_update);
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
    const FluidFloat height_flow = (dheight+dterrain_height) * Fluid::flow_friction;

    const FluidFloat flow = (
            flow_sign*flow_source.fluid_flow[dir] * Fluid::flow_damping +
            height_flow * (FluidFloat(1.0) - Fluid::flow_damping));

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
            applicable_flow = 0;
        }
    } else if (applicable_flow < FluidFloat(0)) {
        // flow is incoming, check that our height is appropriate
        if (meta.terrain_height > neigh_front.fluid_height + neigh_meta.terrain_height) {
            // it can’t go up here
            applicable_flow = 0;
        }
    }

    back.fluid_height -= applicable_flow;
    if (back.fluid_height < FluidFloat(0)) {
#ifndef NDEBUG
        if (std::abs(back.fluid_height) > 1e-6) {
            std::cout << front.fluid_height << std::endl;
            std::cout << neigh_front.fluid_height << std::endl;
            std::cout << back.fluid_height << std::endl;
            std::cout << flow << std::endl;
            std::cout << applicable_flow << std::endl;
            std::cout << flow_sign << std::endl;
            std::cout << dir << std::endl;
            assert(back.fluid_height >= FluidFloat(0));
        } else
#endif
        {
            back.fluid_height = 0.f;
        }
    }

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
}

void Fluid::update_block(const unsigned int x, const unsigned int y)
{
    const unsigned int cy0 = y*m_blocks.m_block_size;
    const unsigned int cy1 = (y+1)*m_blocks.m_block_size;
    const unsigned int cx0 = x*m_blocks.m_block_size;
    const unsigned int cx1 = (x+1)*m_blocks.m_block_size;

    std::array<const FluidCell*, 8> neigh;
    std::array<const FluidCellMeta*, 8> neigh_meta;

    for (unsigned int cy = cy0; cy < cy1; cy++)
    {
        FluidCell *back = m_blocks.cell_back(cx0, cy);
        const FluidCell *front = m_blocks.cell_front(cx0, cy);
        const FluidCellMeta *meta = m_blocks.cell_meta(cx0, cy);
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

            ++back;
            ++front;
            ++meta;
        }
    }
}

void Fluid::worker_impl()
{
    const unsigned int out_of_tasks_limit = m_block_count*m_block_count;

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
        JobType my_job = m_worker_job;
        wakeup_lock.unlock();

        while (1) {
            const unsigned int my_block = m_worker_block_ctr.fetch_add(
                        1,
                        std::memory_order_relaxed);
            if (my_block >= out_of_tasks_limit)
            {
                break;
            }

            const unsigned int x = my_block % m_blocks.m_blocks_per_axis;
            const unsigned int y = my_block / m_blocks.m_blocks_per_axis;
            /*logger.logf(io::LOG_DEBUG, "fluid: %p got %u %u", this, x, y);*/
            switch (my_job) {
            case JobType::PREPARE:
                prepare_block(x, y);
                break;
            case JobType::UPDATE:
                update_block(x, y);
                break;
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

void Fluid::start()
{
    m_blocks.swap_buffers();
    {
        std::unique_lock<std::mutex> lock(m_control_mutex);
        assert(!m_run);
        m_run = true;
    }
    m_control_wakeup.notify_all();
}

void Fluid::to_gl_texture() const
{
    const unsigned int total_cells = m_blocks.m_cells_per_axis*m_blocks.m_cells_per_axis;

    // terrain_height, fluid_height, flowx, flowy
    std::vector<Vector4f> buffer(total_cells);

    {
        std::shared_lock<std::shared_timed_mutex> lock(m_blocks.m_frontbuffer_mutex);
        const FluidCellMeta *meta = m_blocks.cell_meta(0, 0);
        const FluidCell *cell = m_blocks.cell_front(0, 0);
        Vector4f *dest = &buffer.front();
        for (unsigned int i = 0; i < total_cells; i++)
        {
            *dest = Vector4f(meta->terrain_height, cell->fluid_height,
                             cell->fluid_flow[0], cell->fluid_flow[1]);

            ++cell;
            ++meta;
            ++dest;
        }
    }

    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0,
                    m_blocks.m_cells_per_axis,
                    m_blocks.m_cells_per_axis,
                    GL_RGBA,
                    GL_FLOAT,
                    buffer.data());
    engine::raise_last_gl_error();
}

void Fluid::wait_for()
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
