/**********************************************************************
File name: fluid.cpp
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
#include "ffengine/render/fluid.hpp"

#include <map>

namespace engine {

CPUFluid::CPUFluid(const unsigned int terrain_size,
                   const unsigned int grid_size,
                   GLResourceManager &resources,
                   const sim::Fluid &fluidsim):
    FullTerrainRenderer(terrain_size, grid_size),
    m_resources(resources),
    m_fluidsim(fluidsim),
    m_block_size(sim::IFluidSim::block_size),
    m_mat(VBOFormat({
                        VBOAttribute(3),  // position
                        VBOAttribute(4)   // fluid data
                    }))
{
    if ((grid_size-1) != m_block_size) {
        throw std::logic_error("terrain grid_size does not match fluidsim block_size");
    }

    spp::EvaluationContext context(m_resources.shader_library());

    bool success = true;

    success = success && m_mat.shader().attach(
                m_resources.load_shader_checked(":/shaders/fluid/cpu.vert"),
                context);

    success = success && m_mat.shader().attach(
                m_resources.load_shader_checked(":/shaders/fluid/cpu.frag"),
                context);

    m_mat.declare_attribute("position", 0);
    m_mat.declare_attribute("fluiddata", 1);

    success = success && m_mat.link();
    if (!success) {
        throw std::runtime_error("material failed to compile or link");
    }
}

void CPUFluid::copy_into_vertex_cache(Vector4f *dest,
                                      const sim::FluidBlock &src,
                                      const unsigned int x0,
                                      const unsigned int y0,
                                      const unsigned int width,
                                      const unsigned int height,
                                      const unsigned int row_stride,
                                      const unsigned int step,
                                      const float world_x0,
                                      const float world_y0)
{
    for (unsigned int y = y0; y < y0 + height; y += step) {
        const sim::FluidCell *cell = src.local_cell_front(x0, y);
        const sim::FluidCellMeta *meta = src.local_cell_meta(x0, y);

        for (unsigned int x = x0; x < x0 + width; x += step) {
            *dest++ = Vector4f(
                        meta->terrain_height,
                        cell->fluid_height,
                        cell->fluid_flow[0],
                        cell->fluid_flow[1]);
            cell += step;
            meta += step;
        }

        dest += row_stride;
    }
}

void CPUFluid::copy_multi_into_vertex_cache(Vector4f *dest,
                                            const unsigned int x0,
                                            const unsigned int y0,
                                            const unsigned int width,
                                            const unsigned int height,
                                            const unsigned int oversample,
                                            const unsigned int dest_width)
{
    const unsigned int oversampled_width = width * oversample;
    const unsigned int oversampled_height = height * oversample;

    unsigned int ybase = y0;
    unsigned int ydest = 0;

    while (ybase < y0 + oversampled_height) {
        const unsigned int blocky = ybase / m_block_size;
        const unsigned int celly = ybase % m_block_size;

        const unsigned int copy_height = std::min(m_block_size - celly, (height - ydest)*oversample);

        unsigned int xbase = x0;
        unsigned int xdest = 0;

        while (xbase < x0 + oversampled_width) {
            const unsigned int blockx = xbase / m_block_size;
            const unsigned int cellx = xbase % m_block_size;

            const unsigned int copy_width = std::min(m_block_size - cellx, (width - xdest)*oversample);

            const unsigned int row_stride = dest_width - (copy_width+oversample-1)/oversample;

            /*std::cout << "xbase: " << xbase << "; "
                      << "ybase: " << ybase << "; "
                      << "cellx: " << cellx << "; "
                      << "celly: " << celly << "; "
                      << "copy_width: " << copy_width << "; "
                      << "copy_height: " << copy_height << "; "
                      << std::endl;*/

            copy_into_vertex_cache(&dest[ydest*dest_width+xdest],
                                   *m_fluidsim.blocks().block(blockx, blocky),
                                   cellx, celly,
                                   copy_width, copy_height,
                                   row_stride,
                                   oversample,
                                   blockx*m_block_size,
                                   blocky*m_block_size);

            xbase += copy_width;
            if (copy_width % oversample != 0) {
                xbase += (oversample - (copy_width % oversample));
            }

            xdest += (copy_width+oversample-1) / oversample;
        }

        ybase += copy_height;
        if (copy_height % oversample != 0) {
            ybase += (oversample - (copy_height % oversample));
        }

        ydest += (copy_height+oversample-1) / oversample;
    }
}

unsigned int CPUFluid::request_vertex_inject(const float x0f, const float y0f,
                                             const unsigned int oversample,
                                             const unsigned int x,
                                             const unsigned int y)
{
    const unsigned int src_index = y*(m_block_size+3)+x;

    {
        unsigned int existing = m_tmp_index_mapping[src_index];
        if (existing != std::numeric_limits<unsigned int>::max()) {
            return existing;
        }
    }

    const Vector4f &original = m_tmp_fluid_data_cache[src_index];
    Vector3f pos(x0f+x*oversample, y0f+y*oversample, 0);

    if (original[eY] >= 1e-5) {
        pos[eZ] = original[eX] + original[eY];
    } else {
        unsigned int valids = 0;

        int ystart = -1;
        int yend = 1;
        if (y <= 1) {
            ystart = 0;
        }
        if (y >= m_block_size+1) {
            yend = 0;
        }

        int xstart = -1;
        int xend = 1;
        if (x <= 1) {
            xstart = 0;
        }
        if (x >= m_block_size+1) {
            xend = 0;
        }

        Vector4f data;
        for (int yo = ystart; yo <= yend; ++yo) {
            for (int xo = xstart; xo <= xend; ++xo) {
                if (xo == yo && xo == 0) {
                    continue;
                }

                const Vector4f &fluid_data = m_tmp_fluid_data_cache[(y+yo)*(m_block_size+3)+x+xo];
                if (fluid_data[eY] >= 1e-5) {
                    valids += 1;
                    data += fluid_data;
                }
            }
        }

        if (valids == 0) {
            return std::numeric_limits<unsigned int>::max();
        }

        data /= (float)valids;

        float height = data[eX] + data[eY];
        // if terrain size < average neighbour fluid height
        /*if (original[eX] < height) {
            return std::numeric_limits<unsigned int>::max();
        }*/
        pos[eZ] = height;
    }

    unsigned int index = m_tmp_vertex_data.size();
    m_tmp_vertex_data.emplace_back(pos, original);
    m_tmp_index_mapping[src_index] = index;

    return index;
}

void CPUFluid::produce_geometry(const unsigned int blockx,
                                const unsigned int blocky,
                                const unsigned int world_size,
                                const unsigned int oversample)
{
    const unsigned int fcache_size = m_block_size+3;

    m_tmp_fluid_data_cache.resize(fcache_size*fcache_size, Vector4f());
    m_tmp_index_mapping.resize(m_tmp_fluid_data_cache.size(),
                               std::numeric_limits<unsigned int>::max());

    Vector4f *dest = &m_tmp_fluid_data_cache[0];
    unsigned int x0 = blockx*m_block_size;
    unsigned int y0 = blocky*m_block_size;
    unsigned int width = fcache_size;
    unsigned int height = fcache_size;

    bool west_edge = false;
    bool north_edge = false;
    bool south_edge = false;
    bool east_edge = false;

    if (x0 + world_size >= m_fluidsim.blocks().cells_per_axis()-1) {
        width -= 2;
        west_edge = true;
    }

    if (y0 + world_size >= m_fluidsim.blocks().cells_per_axis()-1) {
        height -= 2;
        north_edge = true;
    }

    if (x0 == 0) {
        dest += 1;
        width -= 1;
        east_edge = true;
    } else {
        x0 -= oversample;
    }
    if (y0 == 0) {
        dest += fcache_size;
        height -= 1;
        south_edge = true;
    } else {
        y0 -= oversample;
    }

    /* std::cout << oversample << std::endl; */
    copy_multi_into_vertex_cache(dest, x0, y0, width, height, oversample, fcache_size);

    if (north_edge) {
        for (unsigned int x = 0; x < m_block_size; ++x) {
            const Vector4f &ref = m_tmp_fluid_data_cache[(fcache_size-3)*fcache_size+x+1];
            m_tmp_fluid_data_cache[(fcache_size-2)*fcache_size+x+1] = ref;
            m_tmp_fluid_data_cache[(fcache_size-1)*fcache_size+x+1] = ref;
        }
    }

    if (west_edge) {
        for (unsigned int y = 0; y < m_block_size; ++y) {
            const Vector4f &ref = m_tmp_fluid_data_cache[(y+1)*fcache_size+fcache_size-3];
            m_tmp_fluid_data_cache[(y+1)*fcache_size+fcache_size-2] = ref;
            m_tmp_fluid_data_cache[(y+1)*fcache_size+fcache_size-1] = ref;
        }
    }

    if (west_edge || north_edge) {
        const Vector4f &ref_west = m_tmp_fluid_data_cache[m_block_size*fcache_size+fcache_size-3];
        const Vector4f &ref_north = m_tmp_fluid_data_cache[(fcache_size-3)*fcache_size+m_block_size];
        m_tmp_fluid_data_cache[(fcache_size-2)*fcache_size+fcache_size-2] = (ref_west + ref_north) / 2.f;
    }

    if (south_edge) {
        for (unsigned int x = 0; x < m_block_size; ++x) {
            const Vector4f &ref = m_tmp_fluid_data_cache[fcache_size+x+1];
            m_tmp_fluid_data_cache[x+1] = ref;
        }
    }

    if (east_edge) {
        for (unsigned int y = 0; y < m_block_size; ++y) {
            const Vector4f &ref = m_tmp_fluid_data_cache[(y+1)*fcache_size+1];
            m_tmp_fluid_data_cache[(y+1)*fcache_size] = ref;
        }
    }

    const float x0f = float(blockx*m_block_size) - oversample;
    const float y0f = float(blocky*m_block_size) - oversample;

    // in this run, we add all the other vertices and also the faces
    for (unsigned int y = 2; y < m_block_size+2; ++y) {
        for (unsigned int x = 2; x < m_block_size+2; ++x) {
            unsigned int this_index = request_vertex_inject(
                        x0f, y0f, oversample,
                        x, y);
            unsigned int left_index = request_vertex_inject(
                        x0f, y0f, oversample,
                        x-1, y);
            unsigned int below_index = request_vertex_inject(
                        x0f, y0f, oversample,
                        x, y-1);
            unsigned int below_left_index = request_vertex_inject(
                        x0f, y0f, oversample,
                        x-1, y-1);

            bool this_valid = this_index != std::numeric_limits<unsigned int>::max();
            bool left_valid = left_index != std::numeric_limits<unsigned int>::max();
            bool below_valid = below_index != std::numeric_limits<unsigned int>::max();
            bool below_left_valid = below_left_index != std::numeric_limits<unsigned int>::max();

            if (this_valid && left_valid && below_valid && below_left_valid) {
                const float this_height = std::get<0>(m_tmp_vertex_data[this_index])[eZ];
                const float left_height = std::get<0>(m_tmp_vertex_data[left_index])[eZ];
                const float below_height = std::get<0>(m_tmp_vertex_data[below_index])[eZ];
                const float below_left_height = std::get<0>(m_tmp_vertex_data[below_left_index])[eZ];

                if (abs(this_height - below_left_height) > abs(left_height - below_height)) {
                    m_tmp_index_data.push_back(this_index);
                    m_tmp_index_data.push_back(left_index);
                    m_tmp_index_data.push_back(below_index);

                    m_tmp_index_data.push_back(below_index);
                    m_tmp_index_data.push_back(left_index);
                    m_tmp_index_data.push_back(below_left_index);
                } else {
                    m_tmp_index_data.push_back(below_index);
                    m_tmp_index_data.push_back(this_index);
                    m_tmp_index_data.push_back(below_left_index);

                    m_tmp_index_data.push_back(below_left_index);
                    m_tmp_index_data.push_back(this_index);
                    m_tmp_index_data.push_back(left_index);
                }
            } else if (this_valid && left_valid && below_valid) {
                m_tmp_index_data.push_back(this_index);
                m_tmp_index_data.push_back(left_index);
                m_tmp_index_data.push_back(below_index);
            } else if (this_valid && left_valid && below_left_valid) {
                m_tmp_index_data.push_back(below_left_index);
                m_tmp_index_data.push_back(this_index);
                m_tmp_index_data.push_back(left_index);
            } else if (this_valid && below_valid && below_left_valid) {
                m_tmp_index_data.push_back(below_index);
                m_tmp_index_data.push_back(this_index);
                m_tmp_index_data.push_back(below_left_index);
            } else if (below_valid && left_valid && below_left_valid) {
                m_tmp_index_data.push_back(below_index);
                m_tmp_index_data.push_back(left_index);
                m_tmp_index_data.push_back(below_left_index);
            }
        }
    }

    IBOAllocation ibo_alloc(m_mat.ibo().allocate(m_tmp_index_data.size()));
    VBOAllocation vbo_alloc(m_mat.vbo().allocate(m_tmp_vertex_data.size()));

    {
        auto pos_slice = VBOSlice<Vector3f>(vbo_alloc, 0);
        auto fd_slice = VBOSlice<Vector4f>(vbo_alloc, 1);
        for (unsigned int i = 0; i < m_tmp_vertex_data.size(); ++i) {
            pos_slice[i] = std::get<0>(m_tmp_vertex_data[i]);
            fd_slice[i] = std::get<1>(m_tmp_vertex_data[i]);
        }
        vbo_alloc.mark_dirty();
    }

    {
        uint16_t *dest = ibo_alloc.get();
        memcpy(dest, m_tmp_index_data.data(), sizeof(uint16_t)*m_tmp_index_data.size());
        ibo_alloc.mark_dirty();
    }

    m_allocations.emplace_back(std::move(ibo_alloc), std::move(vbo_alloc), world_size);
}

void CPUFluid::sync(RenderContext &context, const FullTerrainNode &fullterrain)
{
    const unsigned int grid_cells = fullterrain.grid_size()-1;
    m_allocations.clear();
    for (const TerrainSlice &slice: fullterrain.slices_to_render()) {
        produce_geometry(slice.basex / grid_cells,
                         slice.basey / grid_cells,
                         slice.lod,
                         slice.lod / grid_cells);
        m_tmp_index_data.clear();
        m_tmp_fluid_data_cache.clear();
        m_tmp_index_mapping.clear();
        m_tmp_vertex_data.clear();
    }
    m_mat.sync();
    m_mat.shader().bind();
    glUniform3fv(m_mat.shader().uniform_location("lod_viewpoint"), 1, context.viewpoint().as_array);
}

void CPUFluid::render(RenderContext &context, const FullTerrainNode &fullterrain)
{
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    m_mat.shader().bind();
    glUniform1f(m_mat.shader().uniform_location("scale_to_radius"), fullterrain.scale_to_radius());
    for (auto &allocs: m_allocations) {
        unsigned int world_size = std::get<2>(allocs);
        glUniform1f(m_mat.shader().uniform_location("chunk_size"), world_size);
        glUniform1f(m_mat.shader().uniform_location("chunk_lod_scale"), world_size / m_block_size);
        context.draw_elements_base_vertex(GL_TRIANGLES, m_mat, std::get<0>(allocs), std::get<1>(allocs).base());
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

}
