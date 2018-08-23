/**********************************************************************
File name: utils.cpp
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
#include "ffengine/common/utils.hpp"

#include "ffengine/io/log.hpp"


namespace ffe {

static io::Logger &nw_logger = io::logging().get_logger("common.utils.NotifiableWorker");
static io::Logger &tp_logger = io::logging().get_logger("common.utils.ThreadPool");


NotifiableWorker::NotifiableWorker():
    m_notified(false),
    m_terminate(false)
{

}

NotifiableWorker::~NotifiableWorker()
{
    tear_down();
}

void NotifiableWorker::worker_thread()
{
    std::unique_lock<std::mutex> state_lock(m_state_mutex);
    nw_logger.logf(io::LOG_DEBUG, "%p started", this);
    while (!m_terminate)
    {
        while (!m_notified) {
            m_wakeup.wait(state_lock);
        }
        m_notified = false;
        state_lock.unlock();
        nw_logger.logf(io::LOG_DEBUG, "%p woke up", this);

        const bool call_again = worker_impl();

        state_lock.lock();
        m_notified = m_notified || call_again;
    }
    nw_logger.logf(io::LOG_DEBUG, "%p stopped", this);
}

void NotifiableWorker::start()
{
    if (m_worker_thread.joinable()) {
        return;
    }
    m_worker_thread = std::thread(std::bind(&NotifiableWorker::worker_thread,
                                            this));
}

void NotifiableWorker::tear_down()
{
    if (m_worker_thread.joinable()) {
        nw_logger.logf(io::LOG_DEBUG, "stopping %p", this);
        {
            std::lock_guard<std::mutex> guard(m_state_mutex);
            m_terminate = true;
        }
        m_worker_thread.join();
    }
}

void NotifiableWorker::notify()
{
    nw_logger.logf(io::LOG_DEBUG, "notifying %p", this);
    {
        std::lock_guard<std::mutex> guard(m_state_mutex);
        m_notified = true;
    }
    m_wakeup.notify_all();
}


ThreadPool::abstract_packaged_task::abstract_packaged_task(
        abstract_packaged_task &&other):
    m_task(std::move(other.m_task)),
    m_call_exec(std::move(other.m_call_exec)),
    m_call_delete(std::move(other.m_call_delete))
{

}

ThreadPool::abstract_packaged_task::~abstract_packaged_task()
{
    if (m_call_delete != nullptr) {
        m_call_delete(this);
    }
}

void ThreadPool::abstract_packaged_task::operator ()()
{
    m_call_exec(this);
}


ThreadPool::ThreadPool():
    ThreadPool(std::thread::hardware_concurrency())
{

}

ThreadPool::ThreadPool(unsigned int workers):
    m_terminated(false)
{
    initialize_workers(workers);
}

ThreadPool::~ThreadPool()
{
    stop_all();
}

void ThreadPool::initialize_workers(unsigned int workers)
{
    tp_logger.logf(io::LOG_INFO, "initialised thread pool (%p) with %u workers",
                   this, workers);
    m_workers.reserve(workers);
    for (unsigned int i = 0; i < workers; i++)
    {
        m_workers.emplace_back(std::bind(&ThreadPool::worker_impl,
                                         this));
    }
}

void ThreadPool::stop_all()
{
    m_terminated = true;
    m_queue_wakeup.notify_all();
    for (auto &worker: m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
}

void ThreadPool::worker_impl()
{
    std::unique_lock<std::mutex> lock(m_queue_mutex, std::defer_lock);
    while (!m_terminated) {
        lock.lock();
        while (m_task_queue.empty()) {
            m_queue_wakeup.wait(lock);
            if (m_terminated) {
                return;
            }
        }
        abstract_packaged_task task(std::move(m_task_queue.front()));
        m_task_queue.pop_front();
        lock.unlock();

        task();
    }
}

ThreadPool &ThreadPool::global()
{
    static ThreadPool instance;
    return instance;
}


bool is_power_of_two(unsigned int n)
{
    if (n == 0) {
        return false;
    }

    while (!(n & 1)) {
        n >>= 1;
    }
    n >>= 1;
    return !n;
}

unsigned int log2_of_pot(unsigned int n)
{
    return __builtin_ctz(n);
}

}
