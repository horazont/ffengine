/**********************************************************************
File name: fluid_native.hpp
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
#ifndef SCC_SIM_FLUID_NATIVE_H
#define SCC_SIM_FLUID_NATIVE_H

#include <atomic>
#include <thread>

#include "ffengine/sim/fluid_base.hpp"

namespace sim {

class NativeFluidSim: public IFluidSim
{
public:
    NativeFluidSim(FluidBlocks &blocks,
                   const Terrain &terrain);
    ~NativeFluidSim() override;

private:
    FluidBlocks &m_blocks;
    const Terrain &m_terrain;
    const unsigned int m_worker_count;

    /* guarded by m_terrain_update_mutex */
    std::mutex m_terrain_update_mutex;
    TerrainRect m_terrain_update;

    /* guarded by m_ocean_level_update_mutex */
    std::mutex m_ocean_level_update_mutex;
    FluidFloat m_ocean_level_update;
    bool m_ocean_level_update_set;

    /* guarded by m_control_mutex */
    std::mutex m_control_mutex;
    std::condition_variable m_control_wakeup;
    bool m_run;

    /* guarded by m_done_mutex */
    std::mutex m_done_mutex;
    std::condition_variable m_done_wakeup;
    bool m_done;

    /* guarded by m_worker_task_mutex */
    std::mutex m_worker_task_mutex;
    std::condition_variable m_worker_wakeup;
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
    FluidFloat m_ocean_level;

protected:
    void coordinator_impl();
    void coordinator_run_workers();

    void sync_terrain(TerrainRect rect);

    void update_active_block(FluidBlock &block);
    void update_inactive_block(FluidBlock &block);

    void worker_impl();

public:
    void start_frame() override;
    void terrain_update(TerrainRect r) override;
    void set_ocean_level(const FluidFloat level) override;
    void wait_for_frame() override;

};

}

#endif
