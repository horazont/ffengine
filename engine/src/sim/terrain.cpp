/**********************************************************************
File name: terrain.cpp
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
#include "engine/sim/terrain.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

#include "engine/common/utils.hpp"
#include "engine/io/log.hpp"


namespace sim {

io::Logger &lod_logger = io::logging().get_logger("sim.terrain.lod");
static io::Logger &tw_logger = io::logging().get_logger("sim.terrain.worker");


const Terrain::height_t Terrain::default_height = 20.f;
const Terrain::height_t Terrain::min_height = 0.f;
const Terrain::height_t Terrain::max_height = 500.f;

Terrain::Terrain(const unsigned int size):
    m_size(size),
    m_heightmap(m_size*m_size, default_height)
{

}

Terrain::~Terrain()
{

}

void Terrain::notify_heightmap_changed() const
{
    m_terrain_updated.emit(TerrainRect(0, 0, m_size, m_size));
}

void Terrain::notify_heightmap_changed(TerrainRect at) const
{
    m_terrain_updated.emit(at);
}

std::shared_lock<std::shared_timed_mutex> Terrain::readonly_field(
        const Terrain::HeightField *&heightmap) const
{
    heightmap = &m_heightmap;
    return std::shared_lock<std::shared_timed_mutex>(m_heightmap_mutex);
}

std::unique_lock<std::shared_timed_mutex> Terrain::writable_field(
        Terrain::HeightField *&heightmap)
{
    heightmap = &m_heightmap;
    return std::unique_lock<std::shared_timed_mutex>(m_heightmap_mutex);
}

void Terrain::from_perlin(const PerlinNoiseGenerator &gen)
{
    std::unique_lock<std::shared_timed_mutex> lock(m_heightmap_mutex);
    for (unsigned int y = 0; y < m_size; y++) {
        for (unsigned int x = 0; x < m_size; x++) {
            m_heightmap[y*m_size+x] = gen.get(Vector2(x, y));
        }
    }
    lock.unlock();
    notify_heightmap_changed();
}

void Terrain::from_sincos(const Vector3f scale)
{
    const float offset = scale[eZ];
    std::unique_lock<std::shared_timed_mutex> lock(m_heightmap_mutex);
    for (unsigned int y = 0; y < m_size; y++) {
        for (unsigned int x = 0; x < m_size; x++) {
            m_heightmap[y*m_size+x] = (sin(x*scale[eX]) + cos(y*scale[eY])) * scale[eZ] + offset;
        }
    }
    lock.unlock();
    notify_heightmap_changed();
}


TerrainWorker::TerrainWorker():
    m_updated_rect(NotARect),
    m_terminated(false)
{

}

TerrainWorker::~TerrainWorker()
{
    tear_down();
}

void TerrainWorker::worker()
{
    std::unique_lock<std::mutex> lock(m_state_mutex);
    while (!m_terminated) {
        while (!m_updated_rect.is_a_rect()) {
            m_wakeup.wait(lock);
            if (m_terminated) {
                return;
            }
        }
        TerrainRect updated_rect = m_updated_rect;
        m_updated_rect = NotARect;
        lock.unlock();

        tw_logger.log(io::LOG_DEBUG) << "woke worker "
                                     << m_worker_thread.get_id()
                                     << " with rect "
                                     << updated_rect
                                     << io::submit;

        worker_impl(updated_rect);

        lock.lock();
    }
}

void TerrainWorker::start()
{
    if (m_worker_thread.joinable()) {
        throw std::logic_error("Worker already running!");
    }
    m_worker_thread = std::thread(std::bind(&TerrainWorker::worker,
                                            this));
    tw_logger.log(io::LOG_INFO) << "new worker: "
                                << m_worker_thread.get_id()
                                << io::submit;
}

void TerrainWorker::tear_down()
{
    if (m_worker_thread.joinable()) {
        {
            std::unique_lock<std::mutex> lock(m_state_mutex);
            m_terminated = true;
        }
        m_wakeup.notify_all();
        tw_logger.log(io::LOG_INFO) << "tearing down worker "
                                    << m_worker_thread.get_id()
                                    << io::submit;
        m_worker_thread.join();
    }
}

void TerrainWorker::notify_update(const TerrainRect &at)
{
    {
        std::unique_lock<std::mutex> lock(m_state_mutex);
        m_updated_rect = bounds(m_updated_rect, at);
    }
    tw_logger.log(io::LOG_DEBUG) << "notifying worker "
                                 << m_worker_thread.get_id()
                                 << io::submit;
    m_wakeup.notify_all();
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
