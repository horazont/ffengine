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

#include "world_command.pb.h"

namespace sim {

static io::Logger &logger = io::logging().get_logger("sim.world");



/* sim::WorldState */

WorldState::WorldState():
    m_terrain(1081),
    m_fluid(m_terrain)
{

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

    // wait for the fluid sim to finish _without_ holding the lock!
    // this allows the UI to render even while the fluid sim is stuck
    std::lock_guard<std::shared_timed_mutex> lock(m_interframe_mutex);
    {
        std::lock_guard<std::mutex> lock(m_op_queue_mutex);
        m_op_queue.swap(m_op_buffer);
    }

    for (auto &op: m_op_buffer)
    {
        op->execute(m_state);
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

        game_frame();

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
