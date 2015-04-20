/**********************************************************************
File name: utils.hpp
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
#ifndef SCC_ENGINE_COMMON_UTILS_HPP
#define SCC_ENGINE_COMMON_UTILS_HPP

#include <cerrno>
#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <thread>

namespace engine {

class NotifiableWorker
{
public:
    NotifiableWorker();
    virtual ~NotifiableWorker();

private:
    std::mutex m_state_mutex;
    bool m_notified;
    bool m_terminate;
    std::condition_variable m_wakeup;

    std::thread m_worker_thread;

private:
    void worker_thread();

protected:
    void start();
    void tear_down();

    /**
     * Actual implementation of the work to do.
     *
     * @return \c true, if it needs to be called again immediately, independent
     * of notifications, \c false otherwise.
     */
    virtual bool worker_impl() = 0;

public:
    void notify();

};


/**
 * Raise the last OS error (as detected by querying errno) as
 * std::system_error.
 */
static inline void raise_last_os_error()
{
    const int err = errno;
    if (err != 0) {
        throw std::system_error(err, std::system_category());
    }
}


bool is_power_of_two(unsigned int n);
unsigned int log2_of_pot(unsigned int n);


}

#endif // SCC_ENGINE_COMMON_UTILS_HPP

