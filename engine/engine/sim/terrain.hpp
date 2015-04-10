#ifndef SCC_SIM_TERRAIN_H
#define SCC_SIM_TERRAIN_H

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


namespace sim {

namespace {

static io::Logger &lod_logger = io::logging().get_logger("sim.terrain.lod");

}


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

    // guarded by m_heightmap_mutex
    mutable std::shared_timed_mutex m_heightmap_mutex;
    HeightField m_heightmap;

    sigc::signal<void> m_terrain_changed;

protected:
    void notify_heightmap_changed();

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

public:
    std::shared_lock<std::shared_timed_mutex> readonly_field(
            const HeightField *&heightmap) const;
    std::unique_lock<std::shared_timed_mutex> writable_field(
            HeightField *&heightmap);

public:
    void from_perlin(const PerlinNoiseGenerator &gen);

};


class MinMaxMapGenerator: public engine::NotifiableWorker
{
public:
    typedef std::tuple<Terrain::height_t, Terrain::height_t> element_t;
    typedef std::vector<element_t> MinMaxField;
    typedef std::vector<MinMaxField> MinMaxFieldLODs;

public:
    MinMaxMapGenerator(Terrain &source, const unsigned int min_lod);
    ~MinMaxMapGenerator() override;

private:
    Terrain &m_source;
    const unsigned int m_min_lod;
    const unsigned int m_max_size;
    const unsigned int m_lod_count;

    mutable std::shared_timed_mutex m_data_mutex;
    MinMaxFieldLODs m_lods;

protected:
    void make_zeroth_map(MinMaxField &scratchpad);
    bool worker_impl() override;

public:
    void notify_changed();
    std::shared_lock<std::shared_timed_mutex> readonly_lods(
            const MinMaxFieldLODs *&fields) const;

};


class NTMapGenerator: public engine::NotifiableWorker
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

    sigc::signal<void> m_field_changed;

protected:
    bool worker_impl() override;

public:
    inline sigc::signal<void> &field_changed()
    {
        return m_field_changed;
    }

    std::shared_lock<std::shared_timed_mutex> readonly_field(
            const NTField *&field) const;

    inline unsigned int size() const
    {
        return m_source.size();
    }

};


template <typename element_t, typename source_t>
class FieldLODifier: public engine::NotifiableWorker
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
    bool worker_impl() override
    {
        std::shared_lock<std::shared_timed_mutex> read_lock(m_data_mutex);
        std::unique_lock<std::shared_timed_mutex> write_lock(m_data_mutex,
                                                             std::defer_lock);

        const Field *source_field = nullptr;
        auto source_lock = m_source.readonly_field(source_field);
        if (m_lods.size() == 0) {
            m_lods.emplace_back(*source_field);
        } else {
            m_lods[0] = *source_field;
        }

        const Field *prev_heightfield = source_field;
        Field next_heightfield;
        unsigned int prev_size = m_size;

        for (unsigned int i = 1; i < m_lod_count; i++)
        {
            const unsigned int this_size = (m_size >> i)+1;
            next_heightfield.resize(this_size*this_size);
            lod_logger.logf(io::LOG_DEBUG, "generating LOD %u (size=%u)", i,
                            this_size);
            element_t *dest_ptr = &next_heightfield.front();

            for (unsigned int y = 0, src_y = 0; y < this_size; y++, src_y += 2)
            {
                for (unsigned int x = 0, src_x = 0; x < this_size; x++, src_x += 2)
                {
                    *dest_ptr++ = (*prev_heightfield)[src_y*prev_size+src_x];
                }
            }
            lod_logger.logf(io::LOG_DEBUG, "generated LOD %d, saving", i);

            if (source_field) {
                source_field = nullptr;
                source_lock.unlock();
                prev_heightfield = nullptr;
            }

            read_lock.unlock();
            // give other threads the chance to acquire the read lock right now
            std::this_thread::yield();
            write_lock.lock();

            if (m_lods.size() <= i) {
                m_lods.emplace_back(std::move(next_heightfield));
            } else {
                m_lods[i].swap(next_heightfield);
            }

            write_lock.unlock();
            // give other threads the chance to acquire the read lock right now
            std::this_thread::yield();
            read_lock.lock();

            lod_logger.logf(io::LOG_DEBUG, "generated and saved LOD %d", i);
            prev_heightfield = &m_lods[i];
            prev_size = this_size;
        }

        return false;
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
