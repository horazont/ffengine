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


namespace ffe {

static io::Logger &logger = io::logging().get_logger("render.fluid.cpu");

/* engine::FluidSlice */

FluidSlice::FluidSlice(IBOAllocation &&ibo_alloc,
                       VBOAllocation &&vbo_alloc,
                       unsigned int size):
    m_ibo_alloc(std::move(ibo_alloc)),
    m_vbo_alloc(std::move(vbo_alloc)),
    m_size(size)
{

}

/* engine::CPUFluid */

CPUFluid::CPUFluid(const unsigned int terrain_size,
                   const unsigned int grid_size,
                   GLResourceManager &resources,
                   const sim::Fluid &fluidsim,
                   RenderPass &transparent_pass,
                   RenderPass &water_pass):
    FullTerrainRenderer(terrain_size, grid_size),
    m_transparent_pass(transparent_pass),
    m_water_pass(water_pass),
    m_resources(resources),
    m_fluidsim(fluidsim),
    m_block_size(sim::IFluidSim::block_size),
    m_lods(log2_of_pot((terrain_size-1)/(grid_size-1))+1),
    m_mat(VBOFormat({
                        VBOAttribute(3),  // position
                        VBOAttribute(4)   // fluid data
                    }))
{
    if ((grid_size-1) != m_block_size) {
        throw std::logic_error("terrain grid_size does not match fluidsim block_size");
    }

    m_slice_cache.resize(m_lods);
    for (unsigned int i = 0; i < m_lods; ++i) {
        const unsigned int logblocks = m_lods - i - 1;
        std::cout << i << " " << logblocks << " " << ((1<<logblocks)*(1<<logblocks)) << std::endl;
        m_slice_cache[i].resize((1<<logblocks)*(1<<logblocks));
        for (unsigned int j = 0; j < m_slice_cache[i].size(); ++j) {
            m_slice_cache[i][j] = std::make_pair(false, nullptr);
        }
    }

    spp::EvaluationContext context(m_resources.shader_library());

    bool success = true;

    {
        MaterialPass &pass = m_mat.make_pass_material(m_transparent_pass);

        success = success && pass.shader().attach(
                    m_resources.load_shader_checked(":/shaders/fluid/cpu.vert"),
                    context);

        success = success && pass.shader().attach(
                    m_resources.load_shader_checked(":/shaders/fluid/cpu.frag"),
                    context);
    }

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

void CPUFluid::invalidate_caches(const unsigned int blockx,
                                 const unsigned int blocky)
{
    unsigned int divisor = 1;
    unsigned int blocks = (1<<(m_lods-1));
    for (unsigned int lod = 0; lod < m_lods; ++lod) {

        const unsigned int lodblockx = blockx / divisor;
        const unsigned int lodblocky = blocky / divisor;

        const bool invalidate_left = (blockx > 0) && ((blockx % divisor) == 0);
        const bool invalidate_below = (blocky > 0) && ((blocky % divisor) == 0);

        m_slice_cache[lod][lodblocky*blocks+lodblockx] = std::make_pair(false, nullptr);

        if (invalidate_left) {
            m_slice_cache[lod][lodblocky*blocks+lodblockx-1] = std::make_pair(false, nullptr);
            if (invalidate_below) {
                m_slice_cache[lod][(lodblocky-1)*blocks+lodblockx-1] = std::make_pair(false, nullptr);
            }
        }
        if (invalidate_below) {
            m_slice_cache[lod][(lodblocky-1)*blocks+lodblockx] = std::make_pair(false, nullptr);
        }

        divisor *= 2;
        blocks /= 2;
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

std::unique_ptr<FluidSlice> CPUFluid::produce_geometry(const unsigned int blockx,
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

    if (m_tmp_vertex_data.empty() || m_tmp_index_data.empty()) {
        return nullptr;
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

    return std::make_unique<FluidSlice>(std::move(ibo_alloc),
                                        std::move(vbo_alloc),
                                        world_size);
}

void CPUFluid::prepare(RenderContext &context,
                       const FullTerrainNode &fullterrain,
                       const FullTerrainNode::Slices &slices)
{
    const sim::FluidBlock *block = m_fluidsim.blocks().block(0, 0);
    for (unsigned int blocky = 0;
         blocky < m_fluidsim.blocks().blocks_per_axis();
         ++blocky)
    {
        for (unsigned int blockx = 0;
             blockx < m_fluidsim.blocks().blocks_per_axis();
             ++blockx)
        {
            if (block->front_meta().active) {
                // invalidate caches for block
                invalidate_caches(blockx, blocky);
            }
            ++block;
        }
    }

    std::vector<FluidSlice*> render_slices = m_render_slices[&context];
    for (const TerrainSlice &slice: slices) {
        const unsigned int lod = slice.lod / m_block_size;
        const unsigned int loglod = log2_of_pot(lod);
        const unsigned int lodblocks = (1<<(m_lods - loglod - 1));
        const unsigned int blockx = slice.basex / m_block_size;
        const unsigned int blocky = slice.basey / m_block_size;
        const unsigned int lodblockx = blockx / lod;
        const unsigned int lodblocky = blocky / lod;

        {
            auto &cache_entry = m_slice_cache[loglod][lodblocky*lodblocks+lodblockx];
            // if valid
            if (cache_entry.first) {
                // if has geometry
                if (cache_entry.second) {
                    render_slices.push_back(cache_entry.second.get());
                }
                continue;
            }
        }

        auto geometry_slice = produce_geometry(blockx,
                                               blocky,
                                               slice.lod,
                                               slice.lod / m_block_size);
        if (geometry_slice) {
            render_slices.push_back(geometry_slice.get());
        }

        m_slice_cache[loglod][lodblocky*lodblocks+lodblockx] = std::make_pair(
                    true,
                    std::move(geometry_slice));

        m_tmp_index_data.clear();
        m_tmp_fluid_data_cache.clear();
        m_tmp_index_mapping.clear();
        m_tmp_vertex_data.clear();
        m_tmp_used_blocks.clear();
    }

    for (auto iter = m_mat.cbegin();
         iter != m_mat.cend();
         ++iter)
    {
        ShaderProgram &shader = iter->second->shader();
        shader.bind();
        glUniform3fv(shader.uniform_location("lod_viewpoint"),
                     1, context.viewpoint().as_array);
        glUniform1f(shader.uniform_location("scale_to_radius"),
                    fullterrain.scale_to_radius());
    }

    m_mat.sync_buffers();
}

void CPUFluid::render(RenderContext &context,
                      const FullTerrainNode &,
                      const FullTerrainNode::Slices &)
{
    for (FluidSlice *slice: m_render_slices[&context]) {
        unsigned int world_size = slice->m_size;
        context.render_all(AABB{}, GL_TRIANGLES, m_mat,
                           slice->m_ibo_alloc,
                           slice->m_vbo_alloc,
                           [world_size, this](MaterialPass &pass){
                               glUniform1f(pass.shader().uniform_location("chunk_size"),
                                           world_size);
                               glUniform1f(pass.shader().uniform_location("chunk_lod_scale"),
                                           world_size / m_block_size);
                           });
    }
}

void CPUFluid::sync(const FullTerrainNode &fullterrain)
{
    m_render_slices.clear();
}

}
