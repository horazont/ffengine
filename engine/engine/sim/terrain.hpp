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
    static constexpr height_t default_height = 0.f;
    static constexpr height_t max_height = 1024.f;
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


class MinMaxMapGenerator: public TerrainWorker
{
public:
    typedef std::tuple<Terrain::height_t, Terrain::height_t> element_t;
    typedef std::vector<element_t> MinMaxField;
    typedef std::vector<MinMaxField> MinMaxFieldLODs;

public:
    MinMaxMapGenerator(Terrain &source);
    ~MinMaxMapGenerator() override;

private:
    Terrain &m_source;
    const unsigned int m_max_size;
    const unsigned int m_lod_count;

    mutable std::shared_timed_mutex m_data_mutex;
    MinMaxFieldLODs m_lods;

protected:
    void make_zeroth_map(MinMaxField &scratchpad);
    void update_zeroth_map(const TerrainRect &updated);
    void worker_impl(const TerrainRect &updated) override;

public:
    inline unsigned int lod_count() const
    {
        return m_lod_count;
    }

public:
    std::shared_lock<std::shared_timed_mutex> readonly_lods(
            const MinMaxFieldLODs *&fields) const;

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


template <typename element_t, typename source_t>
class FieldLODifier: public TerrainWorker
{
public:
    typedef std::vector<element_t> Field;
    typedef std::vector<Field> FieldLODs;

public:
    FieldLODifier(source_t &source):
        m_source(source),
        m_size(source.size()),
        m_lod_count(engine::log2_of_pot(m_size-1)+1)
    {
        start();
    }

    ~FieldLODifier()
    {
        tear_down();
    }

private:
    source_t &m_source;
    const unsigned int m_size;
    const unsigned int m_lod_count;

    mutable std::shared_timed_mutex m_data_mutex;
    FieldLODs m_lods;

protected:
    void worker_impl(const TerrainRect &updated) override
    {
        std::shared_lock<std::shared_timed_mutex> read_lock(m_data_mutex);
        std::unique_lock<std::shared_timed_mutex> write_lock(m_data_mutex,
                                                             std::defer_lock);

        const Field *source_field = nullptr;
        auto source_lock = m_source.readonly_field(source_field);

        const Field *prev_heightfield = source_field;
        unsigned int prev_size = m_size;

        // avoid re-allocation which breaks everything!
        m_lods.reserve(m_lod_count-1);

        TerrainRect to_update = updated;

        for (unsigned int i = 1; i < m_lod_count; i++)
        {
            const unsigned int this_size = (m_size >> i)+1;
            lod_logger.logf(io::LOG_DEBUG, "generating LOD %u (size=%u)", i,
                            this_size);

            Field *next_heightfield;
            if ((m_lods.size()+1) <= i) {
                // if this level has not been computed yet, force the rect to
                // full size!
                to_update = TerrainRect(
                            0, 0,
                            this_size, this_size);
                read_lock.unlock();
                write_lock.lock();
                m_lods.emplace_back(Field(this_size*this_size));
                next_heightfield = &m_lods[i-1];
            } else {
                // reduce rect size
                to_update.set_x0(to_update.x0() / 2);
                to_update.set_y0(to_update.y0() / 2);

                // +1 for odd field size
                to_update.set_x1(to_update.x1() / 2 + 1);
                to_update.set_y1(to_update.y1() / 2 + 1);

                next_heightfield = &m_lods[i-1];
                read_lock.unlock();
                write_lock.lock();
            }

            const unsigned int this_width = (to_update.x1() - to_update.x0());
            const unsigned int this_height = (to_update.y1() - to_update.y0());

            for (unsigned int y = 0, src_y = to_update.y0()*2;
                 y < this_height;
                 y++, src_y += 2)
            {
                element_t *dest_ptr = &(*next_heightfield)[(to_update.y0()+y)*this_size+to_update.x0()];
                for (unsigned int x = 0, src_x = to_update.x0()*2;
                     x < this_width;
                     x++, src_x += 2)
                {
                    *dest_ptr++ = (*prev_heightfield)[src_y*prev_size+src_x];
                }
            }

            if (source_field) {
                source_field = nullptr;
                source_lock.unlock();
                prev_heightfield = nullptr;
            }

            write_lock.unlock();
            // give other threads the chance to acquire the read lock right now
            std::this_thread::yield();
            read_lock.lock();

            lod_logger.logf(io::LOG_DEBUG, "generated and saved LOD %d", i);
            prev_heightfield = &m_lods[i-1];
            prev_size = this_size;
        }
    }

public:
    inline std::shared_lock<std::shared_timed_mutex> readonly_lods(
            const FieldLODs *&lods) const
    {
        lods = &m_lods;
        return std::shared_lock<std::shared_timed_mutex>(m_data_mutex);
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
