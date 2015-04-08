#include "engine/sim/terrain.hpp"

#include <cstring>
#include <iostream>

#include "engine/common/utils.hpp"
#include "engine/io/log.hpp"


namespace sim {

io::Logger &lod_logger = io::logging().get_logger("sim.terrain.lod");

Terrain::Terrain(const unsigned int size):
    m_size(size),
    m_lod_count(engine::log2_of_pot(size)),
    m_heightmap(m_size*m_size, default_height),
    m_heightmap_changed(true),
    m_terminate(false),
    m_lod_thread(std::bind(&Terrain::lod_thread, this))
{
    if (!engine::is_power_of_two(m_size)) {
        throw std::runtime_error("Terrain size must be power-of-two");
    }
}

Terrain::~Terrain()
{
    {
        std::lock_guard<std::mutex> guard(m_lod_state_mutex);
        m_terminate = true;
    }
    m_lod_wakeup.notify_all();
    m_lod_thread.join();
}

void Terrain::notify_heightmap_changed()
{
    {
        std::lock_guard<std::mutex> guard(m_lod_state_mutex);
        m_heightmap_changed = true;
    }
    m_lod_wakeup.notify_all();
}

void Terrain::lod_iterate()
{
    std::shared_lock<std::shared_timed_mutex> read_lock(m_lod_data_mutex);
    std::unique_lock<std::shared_timed_mutex> write_lock(m_lod_data_mutex,
                                                         std::defer_lock);

    HeightField *prev_heightfield = &m_heightmap_lods[0];
    HeightField next_heightfield;
    unsigned int prev_size = m_size;

    for (unsigned int i = 1; i < m_lod_count; i++)
    {
        const unsigned int this_size = m_size >> i;
        next_heightfield.resize(this_size*this_size);
        lod_logger.logf(io::LOG_DEBUG, "generating LOD %u (size=%u)", i,
                        this_size);
        height_t *dest_ptr = &next_heightfield.front();

        for (unsigned int y = 0, src_y = 0; y < this_size; y++, src_y += 2)
        {
            for (unsigned int x = 0, src_x = 0; x < this_size; x++, src_x += 2)
            {
                float aggregated_height = 0;
                aggregated_height += (*prev_heightfield)[src_y*prev_size+src_x];
                aggregated_height += (*prev_heightfield)[src_y*prev_size+src_x+1];
                aggregated_height += (*prev_heightfield)[(src_y+1)*prev_size+src_x];
                aggregated_height += (*prev_heightfield)[(src_y+1)*prev_size+src_x+1]; 

                *dest_ptr++ = aggregated_height / 4; 
            }
        }
        lod_logger.logf(io::LOG_DEBUG, "generated LOD %d, saving", i);
        read_lock.unlock();
        // give other threads the chance to acquire the read lock right now
        std::this_thread::yield();
        write_lock.lock();

        if (m_heightmap_lods.size() <= i) {
            m_heightmap_lods.emplace_back(std::move(next_heightfield));
        } else {
            m_heightmap_lods[i].swap(next_heightfield);
        }

        write_lock.unlock();
        // give other threads the chance to acquire the read lock right now
        std::this_thread::yield();
        read_lock.lock();

        lod_logger.logf(io::LOG_DEBUG, "generated and saved LOD %d", i);
        prev_heightfield = &m_heightmap_lods[i];
        prev_size = this_size;
    }
}

void Terrain::lod_thread()
{
    {
        std::unique_lock<std::shared_timed_mutex> lod_data_lock(m_lod_data_mutex);
        m_heightmap_lods.reserve(m_lod_count+1);
    }

    std::unique_lock<std::mutex> lock(m_lod_state_mutex);
    while (!m_terminate) {
        while (!m_heightmap_changed) {
            m_lod_wakeup.wait(lock);
            if (m_terminate) {
                return;
            }
        }
        m_heightmap_changed = false;
        lock.unlock();

        lod_logger.log(io::LOG_DEBUG, "change notification received");

        lod_logger.log(io::LOG_DEBUG, "copying source heightmap");
        // copy current terrain
        {
            const HeightField *heightmap = nullptr;
            std::unique_lock<std::shared_timed_mutex> data_lock(m_lod_data_mutex);
            auto lock = readonly_heightmap(heightmap);
            if (m_heightmap_lods.empty()) {
                m_heightmap_lods.emplace_back(*heightmap);
            } else {
                m_heightmap_lods[0] = *heightmap;
            }
        }
        lod_logger.log(io::LOG_DEBUG, "starting LOD iteration...");
        lod_iterate();
        lod_logger.log(io::LOG_DEBUG, "LOD iteration done!");

        lock.lock();
    }
}

std::shared_lock<std::shared_timed_mutex> Terrain::readonly_heightmap(
        const Terrain::HeightField *&heightmap) const
{
    heightmap = &m_heightmap;
    return std::shared_lock<std::shared_timed_mutex>(m_heightmap_mutex);
}

std::shared_lock<std::shared_timed_mutex> Terrain::readonly_lods(
        const HeightFieldLODs *&lods) const
{
    lods = &m_heightmap_lods;
    return std::shared_lock<std::shared_timed_mutex>(m_lod_data_mutex);
}

std::unique_lock<std::shared_timed_mutex> Terrain::writable_heightmap(
        Terrain::HeightField *&heightmap)
{
    heightmap = &m_heightmap;
    return std::unique_lock<std::shared_timed_mutex>(m_heightmap_mutex);
}


void copy_heightfield_rect(
        const Terrain::HeightField &src,
        const unsigned int x0,
        const unsigned int y0,
        const unsigned int src_width,
        Terrain::HeightField &dest,
        const unsigned int dest_width,
        const unsigned int dest_height)
{
    for (unsigned int y = 0, src_y = y0; y < dest_height; y++, src_y++) {
        Terrain::height_t *const dest_ptr = &(&dest.front())[y*dest_width];
        const Terrain::height_t *const src_ptr = &src.data()[src_y*src_width+x0];
        memcpy(dest_ptr, src_ptr, sizeof(Terrain::height_t)*dest_width);
    }
}

}
