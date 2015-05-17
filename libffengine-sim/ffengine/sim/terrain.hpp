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
    typedef std::vector<height_t> HeightField;
    typedef std::vector<std::vector<height_t> > HeightFieldLODs;

public:
    Terrain(const unsigned int size);
    ~Terrain();

private:
    const unsigned int m_size;

    // guarded by m_heightmap_mutex
    mutable std::shared_timed_mutex m_heightmap_mutex;
    HeightField m_heightmap;

    mutable sigc::signal<void, TerrainRect> m_terrain_updated;

public:
    inline height_t get(unsigned int x, unsigned int y) const
    {
        std::shared_lock<std::shared_timed_mutex> lock(m_heightmap_mutex);
        return m_heightmap[y*m_size+x];
    }

    inline void set(unsigned int x, unsigned int y, height_t v)
    {
        {
            std::unique_lock<std::shared_timed_mutex> lock(m_heightmap_mutex);
            m_heightmap[y*m_size+x] = v;
        }
        notify_heightmap_changed();
    }

    inline unsigned int width() const
    {
        return m_size;
    }

    inline unsigned int height() const
    {
        return m_size;
    }

    inline unsigned int size() const
    {
        return m_size;
    }

    inline sigc::signal<void, TerrainRect> &terrain_updated() const
    {
        return m_terrain_updated;
    }

public:
    void notify_heightmap_changed() const;
    void notify_heightmap_changed(TerrainRect at) const;
    std::shared_lock<std::shared_timed_mutex> readonly_field(
            const HeightField *&heightmap) const;
    std::unique_lock<std::shared_timed_mutex> writable_field(
            HeightField *&heightmap);

public:
    void from_perlin(const PerlinNoiseGenerator &gen);
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


void copy_heightfield_rect(const Terrain::HeightField &src,
                           const unsigned int x0,
                           const unsigned int y0,
                           const unsigned int src_width,
                           Terrain::HeightField &dest,
                           const unsigned int dest_width,
                           const unsigned int dest_height);

}

#endif
