#include "engine/sim/terrain.hpp"

#include <cassert>
#include <cstring>
#include <iostream>

#include "engine/common/utils.hpp"
#include "engine/io/log.hpp"


namespace sim {

io::Logger &lod_logger = io::logging().get_logger("sim.terrain.lod");
static io::Logger &tw_logger = io::logging().get_logger("sim.terrain.worker");

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
    m_terrain_updated.emit(TerrainRect(0, 0, m_size, m_size));
}

void Terrain::notify_heightmap_changed(TerrainRect at)
{
    m_terrain_updated.emit(at);
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


MinMaxMapGenerator::MinMaxMapGenerator(Terrain &source):
    m_source(source),
    m_max_size(source.size()-1),
    m_lod_count(engine::log2_of_pot(m_max_size)+1)
{
    // avoid reallocation
    m_lods.reserve(m_lod_count);
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

void MinMaxMapGenerator::update_zeroth_map(const TerrainRect &updated)
{
    TerrainRect r(updated.x0(), updated.y0(),
                  (updated.x1()/2)*2, (updated.y1()/2)*2);
    const unsigned int input_size = m_source.size();
    const unsigned int output_size = m_max_size;

    MinMaxField &dest = m_lods[0];

    const Terrain::HeightField *input = nullptr;
    auto lock = m_source.readonly_field(input);

    for (unsigned int ybase = r.y0(); ybase < r.y1(); ybase++)
    {
        for (unsigned int xbase = r.x0(); xbase < r.x1(); xbase++)
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

            dest[ybase*output_size+xbase] = std::make_tuple(min, max);
        }
    }
}

void MinMaxMapGenerator::worker_impl(const TerrainRect &updated)
{
    std::shared_lock<std::shared_timed_mutex> read_lock(m_data_mutex,
                                                        std::defer_lock);
    std::unique_lock<std::shared_timed_mutex> write_lock(m_data_mutex,
                                                         std::defer_lock);

    read_lock.lock();
    if (m_lods.size() == 0) {
        lod_logger.log(io::LOG_DEBUG, "full recomputation of MinMaxField");
        MinMaxField scratchpad(m_max_size*m_max_size);
        make_zeroth_map(scratchpad);

        read_lock.unlock();
        write_lock.lock();
        m_lods.emplace_back(scratchpad);
    } else {
        lod_logger.log(io::LOG_DEBUG, "partial recomputation of MinMaxField");
        read_lock.unlock();
        write_lock.lock();
        update_zeroth_map(updated);
    }
    write_lock.unlock();
    read_lock.lock();

    MinMaxField *prev_field = &m_lods[0];
    unsigned int prev_size = m_max_size;

    TerrainRect to_update = updated;

    for (unsigned int i = 1; i < m_lod_count; i++)
    {
        const unsigned int this_size = (m_max_size >> i);

        MinMaxField *next_field;
        if (m_lods.size() <= i) {
            lod_logger.logf(io::LOG_DEBUG, "full recomputation of MinMaxField, lod = %u", i);
            // if this level has not been computed yet, force full update
            to_update = TerrainRect(0, 0, this_size, this_size);
            read_lock.unlock();
            write_lock.lock();
            m_lods.emplace_back(MinMaxField(this_size*this_size));
            next_field = &m_lods[i];
        } else {
            lod_logger.logf(io::LOG_DEBUG, "partial recomputation of MinMaxField, lod = %u", i);
            // reduce the rect size
            to_update.set_x0(to_update.x0() / 2);
            to_update.set_y0(to_update.y0() / 2);
            to_update.set_x1(to_update.x1() / 2);
            to_update.set_y1(to_update.y1() / 2);

            next_field = &m_lods[i];

            read_lock.unlock();
            write_lock.lock();
        }

        for (unsigned int y = to_update.y0(); y < to_update.y1(); y++)
        {
            element_t *dest_ptr = &(*next_field)[y*this_size+to_update.x0()];
            for (unsigned int x = to_update.x0(); x < to_update.x1(); x++)
            {
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

        write_lock.unlock();
        read_lock.lock();

        prev_field = next_field;
        prev_size = this_size;
    }
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

void NTMapGenerator::worker_impl(const TerrainRect &updated)
{
    const unsigned int source_size = m_source.size();

    TerrainRect to_update = updated;
    if (to_update.x0() > 0) {
        to_update.set_x0(to_update.x0() - 1);
    }
    if (to_update.y0() > 0) {
        to_update.set_y0(to_update.y0() - 1);
    }
    if (to_update.x1() < source_size) {
        to_update.set_x1(to_update.x1() + 1);
    }
    if (to_update.y1() < source_size) {
        to_update.set_y1(to_update.y1() + 1);
    }

    const unsigned int dst_width = (to_update.x1() - to_update.x0());
    const unsigned int dst_height = (to_update.y1() - to_update.y0());
    const unsigned int src_xoffset = (to_update.x0() == 0 ? 0 : 1);
    const unsigned int src_yoffset = (to_update.y0() == 0 ? 0 : 1);
    const unsigned int src_x0 = to_update.x0() - src_xoffset;
    const unsigned int src_y0 = to_update.y0() - src_yoffset;
    const unsigned int src_width = dst_width
            + (to_update.x0() == 0 ? 0 : 1)
            + (to_update.x1() == source_size ? 0 : 1);
    const unsigned int src_height = dst_height
            + (to_update.y0() == 0 ? 0 : 1)
            + (to_update.y1() == source_size ? 0 : 1);


    Terrain::HeightField source_latch(src_width*src_height);
    // this worker really takes a long time, we will copy the source
    {
        const Terrain::HeightField *heightmap = nullptr;
        auto source_lock = m_source.readonly_field(heightmap);
        for (unsigned int ysrc = src_y0, ylatch = 0;
             ylatch < src_height;
             ylatch++, ysrc++)
        {
            memcpy(&source_latch[ylatch*src_width],
                    &(*heightmap)[ysrc*source_size+src_x0],
                    sizeof(sim::Terrain::height_t)*src_width);
        }

    }

    NTField dest(dst_width*dst_height);

    bool has_ym = to_update.y0() > 0;
    for (unsigned int y = 0;
         y < dst_height;
         y++)
    {
        bool has_xm = to_update.x0() > 0;
        bool has_yp = to_update.y1() < source_size || y < dst_height - 1;
        for (unsigned int x = 0;
             x < dst_width;
             x++)
        {
            const bool has_xp = to_update.x1() < source_size || x < dst_width - 1;
            Vector3f normal(0, 0, 0);
            float tangent_eZ = 0;


            const Terrain::height_t y0x0 = source_latch[(y+src_yoffset)*src_width+x+src_xoffset];
            Terrain::height_t ymx0;
            Terrain::height_t ypx0;
            Terrain::height_t y0xm;
            Terrain::height_t y0xp;

            Vector3f tangent_x1;
            Vector3f tangent_x2;

            Vector3f tangent_y1;
            Vector3f tangent_y2;

            if (has_ym)
            {
                ymx0 = source_latch[(y+src_yoffset-1)*src_width+x+src_xoffset];
                tangent_y1 = Vector3f(0, 1, y0x0 - ymx0);
            }
            if (has_yp)
            {
                ypx0 = source_latch[(y+src_yoffset+1)*src_width+x+src_xoffset];
                tangent_y2 = Vector3f(0, 1, ypx0 - y0x0);
            }
            if (has_xm)
            {
                y0xm = source_latch[(y+src_yoffset)*src_width+x+src_xoffset-1];
                const float z = y0x0 - y0xm;
                tangent_x1 = Vector3f(1, 0, z);
                tangent_eZ += z;
            }
            if (has_xp)
            {
                y0xp = source_latch[(y+src_yoffset)*src_width+x+src_xoffset+1];
                const float z = y0xp - y0x0;
                tangent_x2 = Vector3f(1, 0, z);
                tangent_eZ += z;
                if (!has_xm) {
                    // correct for division by two later on
                    tangent_eZ *= 2;
                }
            } else {
                // correct for division by two later on
                tangent_eZ *= 2;
            }

            assert((has_xm || has_xp) && (has_ym || has_yp));

            if (has_xm && has_ym) {
                // above left face
                normal += tangent_x1 % tangent_y1;
            }
            if (has_xp && has_ym) {
                // above right face
                normal += tangent_x2 % tangent_y1;
            }

            if (has_xm && has_yp) {
                // below left face
                normal += tangent_x1 % tangent_y2;
            }
            if (has_xp && has_yp) {
                // below right face
                normal += tangent_x2 % tangent_y2;
            }

            /* if (!has_xm || !has_ym || !has_yp || !has_xp) {
                std::cerr << x << " " << y << std::endl;
            } */

            normal.normalize();

            dest[y*dst_width+x] = Vector4f(normal, tangent_eZ / 2.);

            has_xm = true;
        }
        has_ym = true;
    }

    /*std::cerr << dst_width*dst_height << std::endl;
    if (dst_width*dst_height < 1000) {
        for (unsigned int y = 0; y < dst_height; y++) {
            for (unsigned int x = 0; x < dst_width; x++) {
                std::cerr << "y = " << y << "; x = " << x << "; " << dest[y*dst_width+x] << std::endl;
            }
        }
    }*/

    {
        std::unique_lock<std::shared_timed_mutex> lock(m_data_mutex);
        m_field.resize(source_size*source_size);
        for (unsigned int ydst = 0, ystore = src_y0+src_yoffset;
             ydst < dst_height;
             ystore++, ydst++)
        {
            memcpy(&m_field[ystore*source_size+src_x0+src_xoffset],
                    &dest[ydst*dst_width],
                    sizeof(Vector4f)*dst_width);
        }
    }

    m_field_updated.emit(updated);
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
