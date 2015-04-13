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

void Terrain::from_sincos(const Vector3f scale)
{
    std::unique_lock<std::shared_timed_mutex> lock(m_heightmap_mutex);
    for (unsigned int y = 0; y < m_size; y++) {
        for (unsigned int x = 0; x < m_size; x++) {
            m_heightmap[y*m_size+x] = (sin(x*scale[eX]) + cos(y*scale[eY])) * scale[eZ];
        }
    }
    lock.unlock();
    notify_heightmap_changed();
}


MinMaxMapGenerator::MinMaxMapGenerator(Terrain &source):
    m_source(source),
    m_max_size(source.size()-1),
    m_lod_count(engine::log2_of_pot(m_max_size)+1)
{
    start();
}

MinMaxMapGenerator::~MinMaxMapGenerator()
{
    tear_down();
}

void MinMaxMapGenerator::make_zeroth_map(MinMaxField &scratchpad)
{
    const unsigned int input_size = m_source.size();
    const unsigned int output_size = m_max_size;

    const Terrain::HeightField *input = nullptr;
    auto lock = m_source.readonly_field(input);

    for (unsigned int ybase = 0; ybase < input_size-1; ybase++)
    {
        for (unsigned int xbase = 0; xbase < input_size-1; xbase++)
        {
            Terrain::height_t min = std::numeric_limits<Terrain::height_t>::max();
            Terrain::height_t max = std::numeric_limits<Terrain::height_t>::min();

            for (unsigned int y = ybase; y < ybase+2; y++) {
                for (unsigned int x = xbase; x < xbase+2; x++) {
                    const Terrain::height_t value = (*input)[y*input_size+x];
                    min = std::min(min, value);
                    max = std::max(max, value);
                }
            }

            scratchpad[ybase*output_size+xbase] = std::make_tuple(min, max);
        }
    }
}

bool MinMaxMapGenerator::worker_impl()
{
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
                Terrain::height_t min, max;
                std::tie(min, max) = (*prev_field)[(2*y)*prev_size+(2*x)];

                Terrain::height_t tmp_min, tmp_max;
                std::tie(tmp_min, tmp_max) = (*prev_field)[(2*y)*prev_size+(2*x+1)];
                min = std::min(tmp_min, min);
                max = std::max(tmp_max, max);

                std::tie(tmp_min, tmp_max) = (*prev_field)[(2*y+1)*prev_size+(2*x+1)];
                min = std::min(tmp_min, min);
                max = std::max(tmp_max, max);

                std::tie(tmp_min, tmp_max) = (*prev_field)[(2*y+1)*prev_size+(2*x)];
                min = std::min(tmp_min, min);
                max = std::max(tmp_max, max);

                *dest_ptr++ = std::make_tuple(min, max);
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


NTMapGenerator::NTMapGenerator(Terrain &source):
    m_source(source)
{
    start();
}

NTMapGenerator::~NTMapGenerator()
{
    tear_down();
}

bool NTMapGenerator::worker_impl()
{
    const unsigned int size = m_source.size();

    Terrain::HeightField source;
    // this worker really takes a long time, we will copy the source
    {
        const Terrain::HeightField *heightmap = nullptr;
        auto source_lock = m_source.readonly_field(heightmap);
        source = *heightmap;
    }

    NTField dest(size*size);


    // first the corners

    // top left corner
    {
        const Terrain::height_t y0x0 = source[0];
        const Terrain::height_t y0xp = source[1];
        const Terrain::height_t ypx0 = source[size];

        const Vector3f tangent_x(1, 0, y0xp - y0x0);
        const Vector3f tangent_y(0, 1, ypx0 - y0x0);

        dest[0] = Vector4f((tangent_x % tangent_y).normalized(),
                           tangent_x[eZ]);
    }

    // top right corner
    {
        const Terrain::height_t y0x0 = source[size-1];
        const Terrain::height_t y0xm = source[size-2];
        const Terrain::height_t ypx0 = source[size+size-1];

        const Vector3f tangent_x(1, 0, y0x0 - y0xm);
        const Vector3f tangent_y(0, 1, ypx0 - y0x0);

        dest[size-1] = Vector4f((tangent_x % tangent_y).normalized(),
                                tangent_x[eZ]);
    }

    // bottom left corner
    {
        const Terrain::height_t y0x0 = source[(size-1)*size];
        const Terrain::height_t y0xp = source[(size-1)*size+1];
        const Terrain::height_t ymx0 = source[(size-2)*size];

        const Vector3f tangent_x(1, 0, y0xp - y0x0);
        const Vector3f tangent_y(0, 1, y0x0 - ymx0);

        dest[(size-1)*size] = Vector4f((tangent_x % tangent_y).normalized(),
                                       tangent_x[eZ]);
    }

    // bottom right corner
    {
        const Terrain::height_t y0x0 = source[(size-1)*size+size-1];
        const Terrain::height_t y0xm = source[(size-1)*size+size-2];
        const Terrain::height_t ymx0 = source[(size-2)*size+size-1];

        const Vector3f tangent_x(1, 0, y0x0 - y0xm);
        const Vector3f tangent_y(0, 1, y0x0 - ymx0);

        dest[(size-1)*size+size-1] = Vector4f((tangent_x % tangent_y).normalized(),
                                              tangent_x[eZ]);
    }

    // then the borders

    // top border
    for (unsigned int x = 1; x < size-1; x++) {
        Vector3f normal(0, 0, 0);

        const Terrain::height_t y0x0 = source[x];
        const Terrain::height_t ypx0 = source[size+x];
        const Terrain::height_t y0xp = source[x+1];
        const Terrain::height_t y0xm = source[x-1];

        const Vector3f tangent_x1(1, 0, y0x0 - y0xm);
        const Vector3f tangent_x2(1, 0, y0xp - y0x0);

        const Vector3f tangent_y(0, 1, ypx0 - y0x0);

        // below left face
        normal += tangent_x1 % tangent_y;
        // below right face
        normal += tangent_x2 % tangent_y;

        normal.normalize();

        Vector3f tangent = tangent_x1 + tangent_x2;
        tangent.normalize();

        dest[x] = Vector4f(normal, tangent[eZ]);
    }

    // bottom border
    for (unsigned int x = 1; x < size-1; x++) {
        Vector3f normal(0, 0, 0);

        const Terrain::height_t y0x0 = source[(size-1)*size+x];
        const Terrain::height_t ymx0 = source[(size-2)*size+x];
        const Terrain::height_t y0xp = source[(size-1)*size+x+1];
        const Terrain::height_t y0xm = source[(size-1)*size+x-1];

        const Vector3f tangent_x1(1, 0, y0x0 - y0xm);
        const Vector3f tangent_x2(1, 0, y0xp - y0x0);

        const Vector3f tangent_y(0, 1, y0x0 - ymx0);

        // above left face
        normal += tangent_x1 % tangent_y;
        // above right face
        normal += tangent_x2 % tangent_y;

        normal.normalize();

        Vector3f tangent = tangent_x1 + tangent_x2;
        tangent.normalize();

        dest[(size-1)*size+x] = Vector4f(normal, tangent[eZ]);
    }

    // left border
    for (unsigned int y = 1; y < size-1; y++) {
        Vector3f normal(0, 0, 0);

        const Terrain::height_t y0x0 = source[y*size];
        const Terrain::height_t ypx0 = source[(y+1)*size];
        const Terrain::height_t ymx0 = source[(y-1)*size];
        const Terrain::height_t y0xp = source[y*size+1];

        const Vector3f tangent_x(1, 0, y0xp - y0x0);

        const Vector3f tangent_y1(0, 1, y0x0 - ymx0);
        const Vector3f tangent_y2(0, 1, ypx0 - y0x0);

        // above right face
        normal += tangent_x % tangent_y1;
        // below right face
        normal += tangent_x % tangent_y2;

        normal.normalize();

        dest[y*size] = Vector4f(normal, tangent_x[eZ]);
    }

    // right border
    for (unsigned int y = 1; y < size-1; y++) {
        Vector3f normal(0, 0, 0);

        const Terrain::height_t y0x0 = source[y*size+(size-1)];
        const Terrain::height_t ypx0 = source[(y+1)*size+(size-1)];
        const Terrain::height_t ymx0 = source[(y-1)*size+(size-1)];
        const Terrain::height_t y0xm = source[y*size+(size-2)];

        const Vector3f tangent_x(1, 0, y0x0 - y0xm);

        const Vector3f tangent_y1(0, 1, y0x0 - ymx0);
        const Vector3f tangent_y2(0, 1, ypx0 - y0x0);

        // above right face
        normal += tangent_x % tangent_y1;
        // below right face
        normal += tangent_x % tangent_y2;

        normal.normalize();

        dest[y*size+size-1] = Vector4f(normal, tangent_x[eZ]);
    }


    // now the interior
    for (unsigned int y = 1; y < size-1; y++) {
        for (unsigned int x = 1; x < size-1; x++) {
            Vector3f normal(0, 0, 0);

            const Terrain::height_t y0x0 = source[y*size+x];
            const Terrain::height_t ymx0 = source[(y-1)*size+x];
            const Terrain::height_t ypx0 = source[(y+1)*size+x];
            const Terrain::height_t y0xm = source[y*size+x-1];
            const Terrain::height_t y0xp = source[y*size+x+1];

            const Vector3f tangent_x1(1, 0, y0x0 - y0xm);
            const Vector3f tangent_x2(1, 0, y0xp - y0x0);

            const Vector3f tangent_y1(0, 1, y0x0 - ymx0);
            const Vector3f tangent_y2(0, 1, ypx0 - y0x0);

            // above left face
            normal += tangent_x1 % tangent_y1;
            // above right face
            normal += tangent_x2 % tangent_y1;

            // below left face
            normal += tangent_x1 % tangent_y2;
            // below right face
            normal += tangent_x2 % tangent_y2;

            normal.normalize();

            const Vector3f tangent = tangent_x1 + tangent_x2;

            dest[y*size+x] = Vector4f(normal, tangent[eZ]);
        }
    }

    std::unique_lock<std::shared_timed_mutex> lock(m_data_mutex);
    m_field.swap(dest);
    lock.unlock();

    m_field_changed.emit();

    return false;
}

std::shared_lock<std::shared_timed_mutex> NTMapGenerator::readonly_field(
        const NTField *&field) const
{
    field = &m_field;
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
