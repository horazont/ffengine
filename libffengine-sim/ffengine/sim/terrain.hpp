/**********************************************************************
File name: terrain.hpp
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
#ifndef SCC_SIM_TERRAIN_H
#define SCC_SIM_TERRAIN_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#include <sigc++/sigc++.h>

#include "noise.h"

#include "ffengine/common/utils.hpp"
#include "ffengine/io/log.hpp"

#include "ffengine/math/perlin.hpp"
#include "ffengine/math/vector.hpp"
#include "ffengine/math/rect.hpp"


namespace sim {

extern io::Logger &lod_logger;

typedef GenericRect<unsigned int> TerrainRect;


class Terrain
{
public:
    typedef float height_t;
    static const height_t default_height;
    static const height_t max_height;
    static const height_t min_height;
    typedef std::vector<Vector3f> Field;

    static const vector_component_x_t HEIGHT_ATTR;
    static const vector_component_y_t SAND_ATTR;

public:
    Terrain(const unsigned int size);
    ~Terrain();

private:
    const unsigned int m_size;

    // guarded by m_heightmap_mutex
    mutable std::shared_timed_mutex m_field_mutex;
    Field m_field;

    mutable sigc::signal<void, TerrainRect> m_heightmap_updated;
    mutable sigc::signal<void, TerrainRect> m_attributes_updated;

public:
    inline unsigned int size() const
    {
        return m_size;
    }

    inline sigc::signal<void, TerrainRect> &heightmap_updated() const
    {
        return m_heightmap_updated;
    }

    inline sigc::signal<void, TerrainRect> &attributes_updated() const
    {
        return m_attributes_updated;
    }

public:
    void notify_heightmap_changed() const;
    void notify_heightmap_changed(TerrainRect at) const;
    void notify_attributes_changed(TerrainRect at) const;
    std::shared_lock<std::shared_timed_mutex> readonly_field(
            const Field *&heightmap) const;
    std::unique_lock<std::shared_timed_mutex> writable_field(
            Field *&heightmap);

public:
    void from_perlin(const PerlinNoiseGenerator &gen);
    void from_noise(const noise::module::Module &gen);
    void from_sincos(const Vector3f scale);

};


class TerrainWorker
{
public:
    TerrainWorker();
    virtual ~TerrainWorker();

private:
    std::mutex m_state_mutex;
    TerrainRect m_updated_rect;
    bool m_terminated;
    std::condition_variable m_wakeup;

    std::thread m_worker_thread;

private:
    void worker();

protected:
    void start();
    void tear_down();
    virtual void worker_impl(const TerrainRect &updated_rect) = 0;

public:
    void notify_update(const TerrainRect &at);

};


class Fluid;
struct FluidCell;

class Sandifier
{
public:
    static const float SAND_FILTER_CONSTANT;
    static const float SAND_FILTER_CUTOFF;

public:
    Sandifier(Terrain &terrain,
              const Fluid &fluid);

private:
    Terrain &m_terrain;
    const Fluid &m_fluid;

    unsigned int m_curr_y;

    std::array<std::vector<Vector3f>, 3> m_rows;

    std::vector<float> m_dest_row;

private:
    void fetch_fluid_info(const unsigned int x,
                          std::array<const FluidCell*, 9> &dest);
    void fetch_row(const unsigned int y,
                   const Terrain::Field &src,
                   std::vector<Vector3f> &height_dest);
    std::tuple<unsigned int, unsigned int> step();

public:
    void run_steps();

};


std::pair<bool, float> lookup_height(const Terrain::Field &field,
                   const unsigned int terrain_size,
                   const float x,
                   const float y);

}

#endif
