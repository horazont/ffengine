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

#include "engine/common/utils.hpp"
#include "engine/io/log.hpp"

#include "engine/math/perlin.hpp"
#include "engine/math/vector.hpp"
#include "engine/math/rect.hpp"


namespace sim {

extern io::Logger &lod_logger;

typedef GenericRect<unsigned int> TerrainRect;


class Terrain
{
public:
    typedef float height_t;
    static constexpr height_t default_height = 20.f;
    static constexpr height_t max_height = 500.f;
    static constexpr height_t min_height = 0.f;
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

    sigc::signal<void> m_terrain_changed;
    sigc::signal<void, TerrainRect> m_terrain_updated;

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

    inline sigc::signal<void> &terrain_changed()
    {
        return m_terrain_changed;
    }

    inline sigc::signal<void, TerrainRect> &terrain_updated()
    {
        return m_terrain_updated;
    }

public:
    void notify_heightmap_changed();
    void notify_heightmap_changed(TerrainRect at);
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


class NTMapGenerator: public TerrainWorker
{
public:
    typedef Vector4f element_t;
    typedef std::vector<element_t> NTField;

public:
    NTMapGenerator(Terrain &source);
    ~NTMapGenerator() override;

private:
    Terrain &m_source;

    mutable std::shared_timed_mutex m_data_mutex;
    NTField m_field;

    sigc::signal<void, TerrainRect> m_field_updated;

protected:
    void worker_impl(const TerrainRect &updated) override;

public:
    inline sigc::signal<void, TerrainRect> &field_updated()
    {
        return m_field_updated;
    }

    std::shared_lock<std::shared_timed_mutex> readonly_field(
            const NTField *&field) const;

    inline unsigned int size() const
    {
        return m_source.size();
    }

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
