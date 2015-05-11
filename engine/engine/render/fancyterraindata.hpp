/**********************************************************************
File name: fancyterraindata.hpp
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
#ifndef SCC_ENGINE_RENDER_FANCYTERRAINDATA_H
#define SCC_ENGINE_RENDER_FANCYTERRAINDATA_H

#include "engine/math/shapes.hpp"

#include "engine/sim/terrain.hpp"

#define DISABLE_QUADTREE

namespace engine {

class NTMapGenerator: public sim::TerrainWorker
{
public:
    typedef Vector4f element_t;
    typedef std::vector<element_t> NTField;

public:
    NTMapGenerator(const sim::Terrain &source);
    ~NTMapGenerator() override;

private:
    const sim::Terrain &m_source;

    mutable std::shared_timed_mutex m_data_mutex;
    NTField m_field;

    sigc::signal<void, sim::TerrainRect> m_field_updated;

protected:
    void worker_impl(const sim::TerrainRect &updated) override;

public:
    inline sigc::signal<void, sim::TerrainRect> &field_updated()
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

/**
 * A helper class to provide data which is derived from the main heightmap in
 * near realtime.
 *
 * It makes sure that the data providers get notified about heightmap changes
 * and update as soon as possible.
 *
 * A main usecase is with FancyTerrainNode, which requires an instance of this
 * class to render the terrain.
 */
class FancyTerrainInterface
{
public:
    FancyTerrainInterface(const sim::Terrain &terrain,
                          const unsigned int grid_size);
    ~FancyTerrainInterface();

private:
    const unsigned int m_grid_size;

    const sim::Terrain &m_terrain;
    NTMapGenerator m_terrain_nt;

    sigc::connection m_terrain_nt_conn;

    std::vector<sigc::connection> m_any_updated_conns;

    sigc::signal<void> m_field_updated;

protected:
    void any_updated(const sim::TerrainRect &at);

public:
    inline unsigned int size() const
    {
        return m_terrain.size();
    }

    inline unsigned int grid_size() const
    {
        return m_grid_size;
    }

    inline const sim::Terrain &terrain()
    {
        return m_terrain;
    }

    inline NTMapGenerator &ntmap()
    {
        return m_terrain_nt;
    }

    inline sigc::signal<void> &field_updated()
    {
        return m_field_updated;
    }

    std::tuple<Vector3f, Vector3f, bool> hittest_quadtree(
            const Ray &ray,
            const unsigned int lod);
    std::tuple<Vector3f, bool> hittest(const Ray &ray);

};

std::tuple<Vector3f, bool> isect_terrain_ray(
        const Ray &ray,
        const unsigned int size,
        const sim::Terrain::HeightField &field);

}

#endif
