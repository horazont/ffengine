#ifndef SCC_SIM_TERRAIN_H
#define SCC_SIM_TERRAIN_H

#include <condition_variable>
#include <cstdint>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#include <sigc++/sigc++.h>

namespace sim {

class Terrain
{
public:
    typedef float height_t;
    static constexpr height_t default_height = 20.f;
    typedef std::vector<height_t> HeightField;
    typedef std::vector<std::vector<height_t> > HeightFieldLODs;

public:
    Terrain(const unsigned int size);
    ~Terrain();

private:
    const unsigned int m_size;
    const unsigned int m_lod_count;

    // guarded by m_heightmap_mutex
    mutable std::shared_timed_mutex m_heightmap_mutex;
    HeightField m_heightmap;

    // guarded by m_lod_state_mutex
    std::mutex m_lod_state_mutex;
    bool m_heightmap_changed;
    bool m_terminate;
    std::condition_variable m_lod_wakeup;

    // guarded by m_lod_data_mutex
    mutable std::shared_timed_mutex m_lod_data_mutex;
    std::vector<HeightField> m_heightmap_lods;

    // owned by thread owning Terrain
    std::thread m_lod_thread;

protected:
    void notify_heightmap_changed();
    void lod_iterate();
    void lod_thread();

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

public:
    std::shared_lock<std::shared_timed_mutex> readonly_heightmap(
            const HeightField *&heightmap) const;
    std::shared_lock<std::shared_timed_mutex> readonly_lods(
            const HeightFieldLODs *&lods) const;
    std::unique_lock<std::shared_timed_mutex> writable_heightmap(
            HeightField *&heightmap);

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
