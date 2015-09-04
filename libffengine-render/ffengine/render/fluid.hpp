/**********************************************************************
File name: fluid.hpp
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
#ifndef SCC_RENDER_FLUID_H
#define SCC_RENDER_FLUID_H

#include "ffengine/sim/fluid.hpp"

#include "ffengine/gl/resource.hpp"

#include "ffengine/render/fullterrain.hpp"

namespace engine {

class CPUFluid: public FullTerrainRenderer
{
public:
    CPUFluid(const unsigned int terrain_size,
             const unsigned int grid_size,
             GLResourceManager &resources,
             const sim::Fluid &fluidsim);

private:
    GLResourceManager &m_resources;
    const sim::Fluid &m_fluidsim;
    const unsigned int m_block_size;

    Material m_mat;

    std::vector<std::tuple<IBOAllocation, VBOAllocation, unsigned int> > m_allocations;

    std::vector<Vector4f> m_tmp_fluid_data_cache;
    std::vector<unsigned int> m_tmp_index_mapping;
    std::vector<std::tuple<Vector3f, Vector4f> > m_tmp_vertex_data;
    std::vector<uint16_t> m_tmp_index_data;

protected:
    void copy_into_vertex_cache(Vector4f *dest,
                                const sim::FluidBlock &src,
                                const unsigned int x0,
                                const unsigned int y0,
                                const unsigned int width,
                                const unsigned int height,
                                const unsigned int row_stride,
                                const unsigned int step,
                                const float world_x0,
                                const float world_y0);

    void copy_multi_into_vertex_cache(Vector4f *dest,
                                      const unsigned int x0,
                                      const unsigned int y0,
                                      const unsigned int width,
                                      const unsigned int height,
                                      const unsigned int oversample, const unsigned int dest_width);


    unsigned int request_vertex_inject(const float x0f, const float y0f, const unsigned int oversample,
                                       const unsigned int x, const unsigned int y);

    void produce_geometry(const unsigned int blockx,
                          const unsigned int blocky,
                          const unsigned int world_size,
                          const unsigned int oversample);

public:
    void sync(RenderContext &context,
              const FullTerrainNode &fullterrain) override;
    void render(RenderContext &context,
                const FullTerrainNode &fullterrain) override;

};

}

#endif
