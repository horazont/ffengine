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

/**
 * Thread-safely queue signals emitted from sig11 signals and allow later
 * replay.
 *
 * The SignalQueue offers handy methods to capture signals emitted from any
 * thread and store the calls (not the results!) internally for later
 * evaluation.
 *
 * The replay() method then actually executes the calls stored internally and
 * clears the queue.
 */
class SignalQueue
{
private:
    std::mutex m_queue_mutex;
    std::vector<std::function<void()> > m_queue;

    std::vector<std::function<void()> > m_tmp_queue;

public:
    /**
     * Replay the currently queued calls in the current thread.
     *
     * This method by itself is thread-safe. However, the functions it calls
     * may not be -- depending on what is connected using this queue. This
     * function possibly calls all receivers connected using connect_queued().
     */
    void replay();

    /**
     * Connect a receiver to a signal using this queue.
     *
     * Whenever the signal emits, the arguments it sent are captured by this
     * object and stored internally. The call is not immediately forwarded to
     * the given receiver. Instead, the calls are forwarded to their respective
     * receivers when replay() is called.
     */
    template <typename callable_t, typename... arg_ts>
    inline sig11::connection_guard<void(arg_ts...)> connect_queued [[gnu::warn_unused_result]] (
            sig11::signal<void(arg_ts...)> &signal,
            callable_t &&receiver)
    {
        // make a copy of the receiver
        std::function<void(arg_ts...)> receiver_func(receiver);
        // take arguments by copy
        return connect(signal, [receiver_func, this](arg_ts... args){
            std::lock_guard<std::mutex> guard(m_queue_mutex);
            m_queue.emplace_back(std::bind(receiver_func, args...));
        });
    }

};


}

#endif
