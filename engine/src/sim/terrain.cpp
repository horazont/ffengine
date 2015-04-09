#include "engine/sim/terrain.hpp"

#include <cstring>
#include <iostream>

#include "engine/common/utils.hpp"
#include "engine/io/log.hpp"


namespace sim {

io::Logger &lod_logger = io::logging().get_logger("sim.terrain.lod");

Terrain::Terrain(const unsigned int size):
    m_size(size),
    m_heightmap(m_size*m_size, default_height)
{
    if (!engine::is_power_of_two(m_size-1)) {
        throw std::runtime_error("Terrain size must be power-of-two plus one");
    }
}

Terrain::~Terrain()
{

}

void Terrain::notify_heightmap_changed()
{
    m_terrain_changed.emit();
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


MinMaxMapGenerator::MinMaxMapGenerator(
        Terrain &source,
        const unsigned int min_lod):
    m_source(source),
    m_min_lod(min_lod),
    m_max_size((source.size()-1) / (m_min_lod-1)),
    m_lod_count(engine::log2_of_pot(m_max_size))
{

}

void MinMaxMapGenerator::make_zeroth_map(MinMaxField &scratchpad)
{
    const unsigned int input_size = m_source.size();
    const unsigned int output_size = m_max_size;

    for (unsigned int ybase = 0, ychk = 0; ybase < input_size; ybase += (m_min_lod-1), ychk++)
    {
        for (unsigned int xbase = 0, xchk = 0; xbase < input_size; xbase += (m_min_lod-1), xchk++)
        {
            Terrain::height_t min = std::numeric_limits<Terrain::height_t>::max();
            Terrain::height_t max = std::numeric_limits<Terrain::height_t>::min();

            for (unsigned int y = ybase; y < ybase+m_min_lod; y++) {
                for (unsigned int x = xbase; x < xbase+m_min_lod; x++) {
                    const Terrain::height_t value = m_input[y*input_size+x];
                    min = std::min(min, value);
                    max = std::max(max, value);
                }
            }

            scratchpad[ychk*output_size+xchk] = std::make_tuple(min, max);
        }
    }
}

bool MinMaxMapGenerator::worker_impl()
{
    // copy input map
    {
        const Terrain::HeightField *heightmap = nullptr;
        auto lock = m_source.readonly_field(heightmap);
        m_input = *heightmap;
    }

    std::shared_lock<std::shared_timed_mutex> read_lock(m_data_mutex,
                                                        std::defer_lock);
    std::unique_lock<std::shared_timed_mutex> write_lock(m_data_mutex,
                                                         std::defer_lock);

    MinMaxField scratchpad(m_max_size*m_max_size);
    make_zeroth_map(scratchpad);

    write_lock.lock();
    if (m_lods.size() == 0) {
        m_lods.emplace_back(scratchpad);
    } else {
        m_lods[0].swap(scratchpad);
    }
    write_lock.unlock();
    read_lock.lock();

    MinMaxField *prev_field = &m_lods[0];
    unsigned int prev_size = m_max_size;

    for (unsigned int i = 1; i < m_lod_count; i++)
    {
        const unsigned int this_size = (m_max_size >> i);
        scratchpad.resize(this_size*this_size);

        element_t *dest_ptr = &scratchpad.front();
        for (unsigned int y = 0; y < this_size; y++) {
            for (unsigned int x = 0; x < this_size; x++) {
                *dest_ptr++ = (*prev_field)[2*y*prev_size+(2*x)];
            }
        }

        read_lock.unlock();
        write_lock.lock();

        if (m_lods.size() <= i) {
            m_lods.emplace_back(scratchpad);
        } else {
            m_lods[i].swap(scratchpad);
        }

        write_lock.unlock();
        read_lock.lock();

        prev_field = &m_lods[i];
        prev_size = this_size;
    }

    return false;
}

std::shared_lock<std::shared_timed_mutex> MinMaxMapGenerator::readonly_lods(
        const MinMaxMapGenerator::MinMaxFieldLODs *&fields) const
{
    fields = &m_lods;
    return std::shared_lock<std::shared_timed_mutex>(m_data_mutex);
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
