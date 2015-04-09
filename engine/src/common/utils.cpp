#include "engine/common/utils.hpp"

#include "engine/io/log.hpp"


namespace engine {

static io::Logger &nw_logger = io::logging().get_logger("engine.common.utils.NotifiableWorker");


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
