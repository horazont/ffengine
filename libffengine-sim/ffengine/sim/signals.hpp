/**********************************************************************
File name: signals.hpp
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
#ifndef SCC_SIM_SIGNALS_H
#define SCC_SIM_SIGNALS_H

#include <sig11/sig11.hpp>


namespace sim {

template <typename callable_t, typename... arg_ts>
static inline sig11::connection_guard<void(arg_ts...)> connect_queued_locked [[gnu::warn_unused_result]] (
        sig11::signal<void(arg_ts...)> &signal,
        callable_t &&receiver,
        std::vector<std::function<void()> > &destination,
        std::mutex &destination_mutex)
{
    std::function<void(arg_ts...)> receiver_func(receiver);
    return connect(signal, [receiver_func, &destination, &destination_mutex](arg_ts... args){
        std::lock_guard<std::mutex> guard(destination_mutex);
        destination.emplace_back(std::bind(receiver_func, args...));
    });
}

}

#endif
