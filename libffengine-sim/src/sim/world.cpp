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
#include "ffengine/sim/world.hpp"

#include "world_command.pb.h"

namespace sim {

/* sim::WorldState */

WorldState::WorldState():
    m_terrain(1921),
    m_fluid(m_terrain),
    m_graph(m_objects)
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


}
