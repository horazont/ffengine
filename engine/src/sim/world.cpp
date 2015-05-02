/**********************************************************************
File name: world.cpp
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
#include "engine/sim/world.hpp"

#include "engine/math/algo.hpp"

#include "world_command.pb.h"

namespace sim {

static io::Logger &logger = io::logging().get_logger("sim.world");


/**
 * Apply a terrain tool using a brush mask.
 *
 * @param field The heightfield to work on
 * @param brush_size Diameter of the brush
 * @param sampled Density map of the brush
 * @param brush_strength Factor which is applied to the density map for each
 * painted pixel.
 * @param terrain_size Size of the terrain, for clipping
 * @param x0 X center for painting
 * @param y0 Y center for painting
 * @param impl Tool implementation
 *
 * @see flatten_tool, raise_tool
 */
template <typename impl_t>
void apply_brush_masked_tool(sim::Terrain::HeightField &field,
                             const unsigned int brush_size,
                             const std::vector<float> &sampled,
                             const float brush_strength,
                             const unsigned int terrain_size,
                             const float x0,
                             const float y0,
                             const impl_t &impl)
{
    const int size = brush_size;
    const float radius = size / 2.f;
    const int terrain_xbase = std::round(x0 - radius);
    const int terrain_ybase = std::round(y0 - radius);

    for (int y = 0; y < size; y++) {
        const int yterrain = y + terrain_ybase;
        if (yterrain < 0) {
            continue;
        }
        if (yterrain >= (int)terrain_size) {
            break;
        }
        for (int x = 0; x < size; x++) {
            const int xterrain = x + terrain_xbase;
            if (xterrain < 0) {
                continue;
            }
            if (xterrain >= (int)terrain_size) {
                break;
            }

            sim::Terrain::height_t &h = field[yterrain*terrain_size+xterrain];
            h = std::max(sim::Terrain::min_height,
                         std::min(sim::Terrain::max_height,
                                  impl.paint(h, brush_strength*sampled[y*size+x])));
        }
    }
}

/**
 * Tool implementation for raising / lowering the terrain based on a brush.
 *
 * For use with apply_brush_masked_tool().
 */
struct raise_tool
{
    sim::Terrain::height_t paint(sim::Terrain::height_t h,
                                 float brush_density) const
    {
        return h + brush_density;
    }
};

/**
 * Tool implementation for flattening the terrain based on a brush.
 *
 * For use with apply_brush_masked_tool().
 */
struct flatten_tool
{
    flatten_tool(sim::Terrain::height_t new_value):
        new_value(new_value)
    {

    }

    sim::Terrain::height_t new_value;

    sim::Terrain::height_t paint(sim::Terrain::height_t h,
                                 float brush_density) const
    {
        return interp_linear(h, new_value, brush_density);
    }
};

/* sim::WorldState */

WorldState::WorldState():
    m_terrain(1081),
    m_fluid(m_terrain)
{

}


/* sim::WorldMutator */

WorldMutator::WorldMutator(WorldState &world):
    m_state(world)
{

}

void WorldMutator::notify_update_terrain_rect(const float xc, const float yc,
                                              const unsigned int brush_size)
{
    const sim::Terrain &terrain = m_state.terrain();
    const float brush_radius = brush_size/2.f;
    sim::TerrainRect r(xc, yc, std::ceil(xc+brush_radius), std::ceil(yc+brush_radius));
    if (xc < std::ceil(brush_radius)) {
        r.set_x0(0);
    } else {
        r.set_x0(xc-std::ceil(brush_radius));
    }
    if (yc < std::ceil(brush_radius)) {
        r.set_y0(0);
    } else {
        r.set_y0(yc-std::ceil(brush_radius));
    }
    if (r.x1() > terrain.size()) {
        r.set_x1(terrain.size());
    }
    if (r.y1() > terrain.size()) {
        r.set_y1(terrain.size());
    }
    terrain.notify_heightmap_changed(r);
}

WorldOperationResult WorldMutator::tf_raise(
        const float xc, const float yc,
        const unsigned int brush_size,
        const std::vector<float> &density_map,
        const float brush_strength)
{
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = m_state.terrain().writable_field(field);
        apply_brush_masked_tool(*field,
                                brush_size, density_map, brush_strength,
                                m_state.terrain().size(),
                                xc, yc,
                                raise_tool());
    }
    notify_update_terrain_rect(xc, yc, brush_size);
    return NO_ERROR;
}

WorldOperationResult WorldMutator::tf_level(
        const float xc, const float yc,
        const unsigned int brush_size,
        const std::vector<float> &density_map,
        const float brush_strength,
        const float reference_height)
{
    {
        sim::Terrain::HeightField *field = nullptr;
        auto lock = m_state.terrain().writable_field(field);
        apply_brush_masked_tool(*field,
                                brush_size, density_map, brush_strength,
                                m_state.terrain().size(),
                                xc, yc,
                                flatten_tool(reference_height));
    }
    notify_update_terrain_rect(xc, yc, brush_size);
    return NO_ERROR;
}


/* sim::WorldOperation */

WorldOperation::~WorldOperation()
{

}


/* sim::AbstractClient */

std::atomic<WorldOperationToken> AbstractClient::m_token_ctr(0);

AbstractClient::~AbstractClient()
{

}

void AbstractClient::send_command(messages::WorldCommand &cmd,
                                  ResultCallback &&callback)
{
    if (callback != nullptr) {
        cmd.set_token(m_token_ctr++);
        std::unique_lock<std::mutex> lock(m_callbacks_mutex);
        m_callbacks[cmd.token()] = std::move(callback);
    } else {
        cmd.clear_token();
    }
    send_command_to_backend(cmd);
}

void AbstractClient::recv_response(const messages::WorldCommandResponse &resp)
{
    WorldOperationToken token = resp.token();
    ResultCallback cb = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_callbacks_mutex);
        auto iter = m_callbacks.find(token);
        if (iter != m_callbacks.end()) {
            cb = std::move(iter->second);
            m_callbacks.erase(iter);
        }
    }
    // TODO: maybe use std::async here, if it seems sensible at some point
    cb(resp.result());
}


/* sim::Server */

Server::Server():
    m_state(),
    m_mutator(m_state),
    m_terminated(false),
    m_game_thread(std::bind(&Server::game_thread, this))
{

}

Server::~Server()
{
    m_terminated = true;
    m_game_thread.join();
}

void Server::game_frame()
{
    m_state.fluid().wait_for();

    {
        std::unique_lock<std::mutex> lock(m_op_queue_mutex);
        m_op_queue.swap(m_op_buffer);
    }

    for (auto &op: m_op_buffer)
    {
        op->execute(m_mutator);
    }

    m_op_buffer.clear();
;
    m_state.fluid().start();
}

void Server::game_thread()
{
    static const std::chrono::microseconds game_frame_duration(16000);
    static const std::chrono::microseconds busywait(100);

    m_state.fluid().start();
    // tnext_frame is always in the future when we are on time
    WorldClock::time_point tnext_frame = WorldClock::now();
    while (!m_terminated)
    {
        WorldClock::time_point tnow = WorldClock::now();
        if (tnext_frame > tnow)
        {
            std::chrono::nanoseconds time_to_sleep = tnext_frame - tnow;
            if (time_to_sleep > busywait) {
                std::this_thread::sleep_for(time_to_sleep - busywait);
            }
            continue;
        }

        {
            std::unique_lock<std::shared_timed_mutex> lock(m_interframe_mutex);
            game_frame();
        }

        tnext_frame += game_frame_duration;
    }
}

void Server::enqueue_op(std::unique_ptr<WorldOperation> &&op)
{
    std::unique_lock<std::mutex> lock(m_op_queue_mutex);
    m_op_queue.emplace_back(std::move(op));
}

Server::SyncSafeLock Server::sync_safe_point()
{
    SyncSafeLock lock(m_interframe_mutex);
    // TODO: for now, this is all we need. when other simulations get added,
    // we will need to make them stop here. Fluid sim has the advantage of
    // being double buffered, so we don’t have to wait for it, only make sure
    // that no new frame gets started and no world state is modified.
    return lock;
}


}
