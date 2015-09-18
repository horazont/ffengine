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
#include "ffengine/sim/terrain.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

#include "ffengine/common/utils.hpp"

#include "ffengine/io/log.hpp"

#include "ffengine/math/algo.hpp"

#include "ffengine/sim/fluid.hpp"


namespace sim {

io::Logger &lod_logger = io::logging().get_logger("sim.terrain.lod");
static io::Logger &tw_logger = io::logging().get_logger("sim.terrain.worker");
static io::Logger &sandifier_logger = io::logging().get_logger("sim.terrain.sandifier");


const Terrain::height_t Terrain::default_height = 20.f;
const Terrain::height_t Terrain::min_height = 0.f;
const Terrain::height_t Terrain::max_height = 500.f;

const vector_component_x_t Terrain::HEIGHT_ATTR = eX;

Terrain::Terrain(const unsigned int size):
    m_size(size),
    m_field(m_size*m_size, Vector3f(default_height, 0, 0))
{

}

Terrain::~Terrain()
{

}

void Terrain::notify_heightmap_changed() const
{
    m_heightmap_updated.emit(TerrainRect(0, 0, m_size, m_size));
}

void Terrain::notify_heightmap_changed(TerrainRect at) const
{
    m_heightmap_updated.emit(at);
}

void Terrain::notify_attributes_changed(TerrainRect at) const
{
    m_attributes_updated.emit(at);
}

std::shared_lock<std::shared_timed_mutex> Terrain::readonly_field(
        const Terrain::Field *&heightmap) const
{
    heightmap = &m_field;
    return std::shared_lock<std::shared_timed_mutex>(m_field_mutex);
}

std::unique_lock<std::shared_timed_mutex> Terrain::writable_field(
        Terrain::Field *&heightmap)
{
    heightmap = &m_field;
    return std::unique_lock<std::shared_timed_mutex>(m_field_mutex);
}

void Terrain::from_perlin(const PerlinNoiseGenerator &gen)
{
    std::unique_lock<std::shared_timed_mutex> lock(m_field_mutex);
    for (unsigned int y = 0; y < m_size; y++) {
        for (unsigned int x = 0; x < m_size; x++) {
            m_field[y*m_size+x][HEIGHT_ATTR] = gen.get(Vector2(x, y));
        }
    }
    lock.unlock();
    notify_heightmap_changed();
}

void Terrain::from_sincos(const Vector3f scale)
{
    const float offset = scale[eZ];
    std::unique_lock<std::shared_timed_mutex> lock(m_field_mutex);
    for (unsigned int y = 0; y < m_size; y++) {
        for (unsigned int x = 0; x < m_size; x++) {
            m_field[y*m_size+x][HEIGHT_ATTR] = (sin(x*scale[eX]) + cos(y*scale[eY])) * scale[eZ] + offset;
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


std::pair<bool, float> lookup_height(
        const Terrain::Field &field,
        const unsigned int terrain_size,
        const float x,
        const float y)
{
    const int terrainx = std::round(x);
    const int terrainy = std::round(y);

    if (terrainx < 0 || terrainx >= (int)terrain_size ||
            terrainy < 0 || terrainy >= (int)terrain_size)
    {
        return std::make_pair(false, 0);
    }

    return std::make_pair(true, field[terrainy*terrain_size+terrainx][Terrain::HEIGHT_ATTR]);
}

Sandifier::Sandifier(Terrain &terrain, const Fluid &fluid):
    m_terrain(terrain),
    m_fluid(fluid),
    m_curr_blockx(0),
    m_curr_blocky(0)
{

}

float Sandifier::blurred(const int xc, const int yc)
{
    float sum = 0;
    float weight_sum = 0;
    for (int y = yc - 2; y <= yc + 2; ++y) {
        for (int x = xc - 2; x <= xc + 2; ++x) {
            const sim::FluidCell *cell = m_fluid.blocks().clamped_cell_front(x, y);
            /*const float d = std::sqrt(
                        sqr((float)x-(float)xc)+sqr((float)y-(float)yc)) / 2.5f;
            const float weight = parzen(d);*/
            const float weight = 1.f;
            weight_sum += weight;
            if (cell->fluid_height > 1e-5) {
                sum += weight;
            }
        }
    }
    return sum / weight_sum;
}

void Sandifier::step()
{
    const unsigned int x0 = m_curr_blockx * IFluidSim::block_size;
    const unsigned int y0 = m_curr_blocky * IFluidSim::block_size;

    bool changed = false;
    {
        Terrain::Field *field;
        auto lock = m_terrain.writable_field(field);
        for (unsigned int y = y0; y < y0+IFluidSim::block_size; ++y)
        {
            Vector3f *dest = &(*field)[y*m_terrain.size()+x0];
            for (unsigned int x = x0; x < x0+IFluidSim::block_size; ++x)
            {
                float waterness = blurred(x, y);
                float &s = (*dest)[Terrain::SAND_ATTR];
                if (s > 0 && waterness == 0) {
                    s = 0; //std::max(0.f, s - 0.2f);
                    changed = true;
                } else if (s < waterness){
                    s = waterness; // std::min(waterness, std::max(s, 0.05f)*1.1f);
                    changed = true;
                }

                dest++;
            }
        }
    }

    if (changed) {
        m_terrain.notify_attributes_changed(
                    TerrainRect(
                        x0,
                        y0,
                        x0+IFluidSim::block_size,
                        y0+IFluidSim::block_size));
    }

    m_curr_blockx += 1;
    if (m_curr_blockx >= m_fluid.blocks().blocks_per_axis()) {
        m_curr_blockx = 0;
        m_curr_blocky += 1;
    }
    if (m_curr_blocky >= m_fluid.blocks().blocks_per_axis()) {
        m_curr_blocky = 0;
    }
}

void Sandifier::run_steps()
{
    sandifier_logger.logf(io::LOG_DEBUG, "sandifying at %d %d", m_curr_blockx, m_curr_blocky);
    for (unsigned int i = 0; i < 4; ++i) {
        step();
    }
}


}
