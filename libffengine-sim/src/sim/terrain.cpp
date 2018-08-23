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


const Terrain::height_t Terrain::default_height = 20.f;
const Terrain::height_t Terrain::min_height = 0.f;
const Terrain::height_t Terrain::max_height = 500.f;

const vector_component_x_t Terrain::HEIGHT_ATTR = eX;
const vector_component_y_t Terrain::SAND_ATTR = eY;

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

void sample_from_perlin(const PerlinNoiseGenerator &gen,
                        const std::size_t size,
                        const unsigned int x0, const unsigned int x1,
                        const unsigned int y0, const unsigned int y1,
                        Terrain::Field &buf)
{
    for (unsigned int y = y0; y < y1; ++y) {
        auto *out = &buf[y * size];
        for (unsigned int x = x0; x < x1; ++x) {
            (*out++)[Terrain::HEIGHT_ATTR] = gen.get(Vector2(x, y));
        }
    }
}

void sample_from_noise(const noise::module::Module &gen,
                       const std::size_t size,
                       const unsigned int x0, const unsigned int x1,
                       const unsigned int y0, const unsigned int y1,
                       Terrain::Field &buf)
{
    for (unsigned int y = y0; y < y1; ++y) {
        auto *out = &buf[y * size];
        for (unsigned int x = x0; x < x1; ++x) {
            (*out++)[Terrain::HEIGHT_ATTR] = gen.GetValue(x, y, 0.f);
        }
    }
}

void Terrain::from_perlin(const PerlinNoiseGenerator &gen)
{
    ffe::ThreadPool &pool = ffe::ThreadPool::global();

    std::unique_lock<std::shared_timed_mutex> lock(m_field_mutex);

    const unsigned int step = std::max(16U, pool.workers());
    std::vector<std::future<void>> tasks;
    tasks.reserve(pool.workers() + 1);
    Terrain::Field &buffer = m_field;
    const std::size_t size = m_size;
    for (unsigned int y0 = 0; y0 < m_size; y0 += step) {
        const unsigned int y1 = std::min(y0 + step, m_size);

        tasks.emplace_back(pool.submit_task(std::packaged_task<void()>([=, &gen, &buffer](){
            sample_from_perlin(gen, size, 0, size, y0, y1, buffer);
        })));
    }

    for (auto &fut: tasks) {
        fut.wait();
    }

    lock.unlock();
    notify_heightmap_changed();
}

void Terrain::from_noise(const noise::module::Module &gen)
{
    ffe::ThreadPool &pool = ffe::ThreadPool::global();

    std::unique_lock<std::shared_timed_mutex> lock(m_field_mutex);

    const unsigned int step = std::max(16U, pool.workers());
    std::vector<std::future<void>> tasks;
    tasks.reserve(pool.workers() + 1);
    Terrain::Field &buffer = m_field;
    const std::size_t size = m_size;
    for (unsigned int y0 = 0; y0 < m_size; y0 += step) {
        const unsigned int y1 = std::min(y0 + step, m_size);

        tasks.emplace_back(pool.submit_task(std::packaged_task<void()>([=, &gen, &buffer](){
            sample_from_noise(gen, size, 0, size, y0, y1, buffer);
        })));
    }

    for (auto &fut: tasks) {
        fut.wait();
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


const float Sandifier::SAND_FILTER_CONSTANT = 0.4f;
const float Sandifier::SAND_FILTER_CUTOFF = 1e-2;


Sandifier::Sandifier(Terrain &terrain, const Fluid &fluid):
    m_terrain(terrain),
    m_fluid(fluid),
    m_curr_y(0),
    m_dest_row(m_terrain.size())
{
    m_rows[0].resize(m_terrain.size()+2);
    m_rows[1].resize(m_rows[0].size());
    m_rows[2].resize(m_rows[0].size());
}

void Sandifier::fetch_fluid_info(const unsigned int x,
                                 std::array<const FluidCell*, 9> &dest)
{
    const int xc = x;
    const int yc = m_curr_y;

    dest[0] = m_fluid.blocks().clamped_cell_front(xc-1, yc-1);
    dest[1] = m_fluid.blocks().clamped_cell_front(xc, yc-1);
    dest[2] = m_fluid.blocks().clamped_cell_front(xc+1, yc-1);

    dest[3] = m_fluid.blocks().clamped_cell_front(xc-1, yc);
    dest[4] = m_fluid.blocks().clamped_cell_front(xc, yc);
    dest[5] = m_fluid.blocks().clamped_cell_front(xc+1, yc);

    dest[6] = m_fluid.blocks().clamped_cell_front(xc-1, yc+1);
    dest[7] = m_fluid.blocks().clamped_cell_front(xc, yc+1);
    dest[8] = m_fluid.blocks().clamped_cell_front(xc+1, yc+1);
}

void Sandifier::fetch_row(const unsigned int y,
                          const Terrain::Field &src,
                          std::vector<Vector3f> &dest)
{
    memcpy(&dest[1], &src[y*m_terrain.size()],
            sizeof(Terrain::Field::value_type)*m_terrain.size());
    dest[0] = dest[1];
    dest[m_terrain.size()] = dest[m_terrain.size()-1];
}

std::tuple<unsigned int, unsigned int> Sandifier::step()
{
    {
        const Terrain::Field *field;
        auto lock = m_terrain.readonly_field(field);
        if (m_curr_y == 0) {
            // load first row and copy it in the prev buffer
            fetch_row(m_curr_y, *field, m_rows[1]);
            m_rows[0] = m_rows[1];
        } else {
            //m_rows[0].swap(m_rows[1]);  // move 1st to 0th
            //m_rows[1].swap(m_rows[2]);  // move 2nd to 1st
            m_rows[0] = std::move(m_rows[1]);
            m_rows[1] = std::move(m_rows[2]);
            m_rows[2].resize(m_rows[0].size());
        }
        if (m_curr_y < m_terrain.size()-1) {
            fetch_row(m_curr_y+1, *field, m_rows[2]);  // load next row
        } else {
            // copy last row
            m_rows[2] = m_rows[1];
        }
    }

    unsigned int min_changed_x = m_terrain.size();
    unsigned int max_changed_x = 0;
    bool changed = false;

    for (unsigned int xc = 0; xc < m_terrain.size(); ++xc) {
        const FluidCell &center_cell = *m_fluid.blocks().clamped_cell_front(xc, m_curr_y);

        /*std::array<const FluidCell*, 9> neighbourhood;
        fetch_fluid_info(x, neighbourhood);*/


        std::vector<Vector3f> *row = &m_rows[0];
        //const FluidCell **cells = &neighbourhood[0];

        const float prev_sandiness = m_rows[1][xc+1][Terrain::SAND_ATTR];
        const float local_height = m_rows[1][xc+1][Terrain::HEIGHT_ATTR];
        m_dest_row[xc] = prev_sandiness;

        float new_value = 0.f;

        if (center_cell.fluid_height >= 0.01) {
            // clamp wet cells to 1
            new_value = 1.f;
        } else {
            float accum = 0;
            float min_value = std::numeric_limits<float>::infinity();
            float max_value = 0;

            for (int yo = -1; yo <= 1; ++yo) {
                for (int xo = -1; xo <= 1; ++xo) {
                    if (xo == 0 && yo == 0) {
                        continue;
                    }

                    const float sandiness = (*row)[xc+1+xo][Terrain::SAND_ATTR];
                    const float value = std::max(0.f, sandiness- std::fabs((*row)[xc+1+xo][Terrain::HEIGHT_ATTR] - local_height) / 1000.f);

                    if (value >= prev_sandiness) {
                        if (value > 0) {
                            min_value = std::min(min_value, value);
                        }
                        accum += value;
                    }
                    max_value = std::max(value, max_value);
                }
                row++;
            }

            min_value = std::max(prev_sandiness, min_value);

            // sand sources
            new_value = std::min(min_value, accum / 5.f);

            if (new_value == max_value) {
                //new_value *= 0.95;
                new_value = 0.f; // filtering will prevent oscillation
            }
        }

        if (prev_sandiness != new_value) {
            min_changed_x = std::min(min_changed_x, xc);
            max_changed_x = std::max(max_changed_x, xc);
            /*if (!center_cell.wet()) {
                std::cout << prev_sandiness << " " << new_value << std::endl;
            }*/
            if (std::fabs(prev_sandiness - new_value) < SAND_FILTER_CUTOFF) {
                m_dest_row[xc] = new_value;
            } else {
                m_dest_row[xc] = SAND_FILTER_CONSTANT * new_value + prev_sandiness * (1-SAND_FILTER_CONSTANT);
            }
        }
    }

    if (min_changed_x <= max_changed_x) {
        Terrain::Field *field;
        auto lock = m_terrain.writable_field(field);
        for (unsigned int x = min_changed_x; x <= max_changed_x; ++x) {
            (*field)[m_curr_y*m_terrain.size() + x][Terrain::SAND_ATTR] = m_dest_row[x];
        }
    }

    m_curr_y += 1;
    if (m_curr_y == m_terrain.size()) {
        m_curr_y = 0;
    }

    return std::make_tuple(min_changed_x, max_changed_x);
}

void Sandifier::run_steps()
{
    unsigned int start_y = m_curr_y;
    unsigned int min_changed_x = m_terrain.size(), max_changed_x = 0;

    const unsigned int rows = (start_y == m_terrain.size()-5 ? 5 : 4);

    for (unsigned int i = 0; i < rows; ++i) {
        unsigned int this_min_changed_x, this_max_changed_x;
        std::tie(this_min_changed_x, this_max_changed_x) = step();

        if (this_min_changed_x <= this_max_changed_x) {
            min_changed_x = std::min(min_changed_x, this_min_changed_x);
            max_changed_x = std::max(max_changed_x, this_max_changed_x);
        }
    }

    if (min_changed_x <= max_changed_x) {
        const TerrainRect changed_rect(min_changed_x, start_y, max_changed_x + 1, start_y+rows);
        std::cout << changed_rect << std::endl;
        m_terrain.notify_attributes_changed(changed_rect);
    }
}


}
