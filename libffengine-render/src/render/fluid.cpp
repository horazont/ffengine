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
                        VBOAttribute(3),  // normal
                        VBOAttribute(3)   // tangent
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
    m_mat.declare_attribute("normal", 1);
    m_mat.declare_attribute("tangent", 2);

    success = success && m_mat.link();
    if (!success) {
        throw std::runtime_error("material failed to compile or link");
    }
}

void CPUFluid::copy_into_vertex_cache(Vector3f *dest,
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
            if (cell->fluid_height > 1e-5) {
                *dest++ = Vector3f(
                            world_x0+x,
                            world_y0+y,
                            cell->fluid_height + meta->terrain_height);
            } else {
                *dest++ = Vector3f(
                            world_x0+x,
                            world_y0+y,
                            NAN);
            }
            cell += step;
            meta += step;
        }

        dest += row_stride;
    }
}

void CPUFluid::copy_multi_into_vertex_cache(Vector3f *dest,
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

void CPUFluid::produce_geometry(const unsigned int blockx,
                                const unsigned int blocky,
                                const unsigned int world_size,
                                const unsigned int oversample)
{
    const unsigned int vcache_size = m_block_size+3;

    IBOAllocation ibo_alloc(m_mat.ibo().allocate(m_block_size*m_block_size*6));
    VBOAllocation vbo_alloc(m_mat.vbo().allocate((m_block_size+1)*(m_block_size+1)));

    std::vector<Vector3f> vertex_cache;
    vertex_cache.resize(vcache_size*vcache_size, Vector3f(0, 0, 0));

    Vector3f *dest = &vertex_cache[0];
    unsigned int x0 = blockx*m_block_size;
    unsigned int y0 = blocky*m_block_size;
    unsigned int width = vcache_size;
    unsigned int height = vcache_size;

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
        dest += vcache_size;
        height -= 1;
        south_edge = true;
    } else {
        y0 -= oversample;
    }

    /* std::cout << oversample << std::endl; */
    copy_multi_into_vertex_cache(dest, x0, y0, width, height, oversample, vcache_size);

    if (north_edge) {
        for (unsigned int x = 0; x < m_block_size; ++x) {
            const Vector3f &ref = vertex_cache[(vcache_size-3)*vcache_size+x+1];
            vertex_cache[(vcache_size-2)*vcache_size+x+1] = ref + Vector3f(0, oversample, 0);
            vertex_cache[(vcache_size-1)*vcache_size+x+1] = ref + Vector3f(0, 2*oversample, 0);
        }
    }

    if (west_edge) {
        for (unsigned int y = 0; y < m_block_size; ++y) {
            const Vector3f &ref = vertex_cache[(y+1)*vcache_size+vcache_size-3];
            vertex_cache[(y+1)*vcache_size+vcache_size-2] = ref + Vector3f(oversample, 0, 0);
            vertex_cache[(y+1)*vcache_size+vcache_size-1] = ref + Vector3f(2*oversample, 0, 0);
        }
    }

    if (west_edge || north_edge) {
        const Vector3f &ref_west = vertex_cache[m_block_size*vcache_size+vcache_size-3];
        const Vector3f &ref_north = vertex_cache[(vcache_size-3)*vcache_size+m_block_size];
        vertex_cache[(vcache_size-2)*vcache_size+vcache_size-2] = Vector3f(
                    ref_west[eX]+oversample,
                    ref_north[eY]+oversample,
                    (ref_west[eZ] + ref_north[eZ]) / 2.f);
    }

    if (south_edge) {
        for (unsigned int x = 0; x < m_block_size; ++x) {
            const Vector3f &ref = vertex_cache[vcache_size+x+1];
            vertex_cache[x+1] = ref + Vector3f(0, -oversample, 0);
        }
    }

    if (east_edge) {
        for (unsigned int y = 0; y < m_block_size; ++y) {
            const Vector3f &ref = vertex_cache[(y+1)*vcache_size+1];
            vertex_cache[(y+1)*vcache_size] = ref + Vector3f(-oversample, 0, 0);
        }
    }

    {
        auto pos_slice = VBOSlice<Vector3f>(vbo_alloc, 0);
        for (unsigned int y = 0; y < m_block_size+1; ++y) {
            for (unsigned int x = 0; x < m_block_size+1; ++x) {
                pos_slice[y*(m_block_size+1)+x] = vertex_cache[(y+1)*vcache_size+x+1];
            }
        }
        vbo_alloc.mark_dirty();
    }

    {
        uint16_t *dest = ibo_alloc.get();
        for (unsigned int y = 0; y < m_block_size; ++y) {
            for (unsigned int x = 0; x < m_block_size; ++x) {
                const unsigned int vertex_base = (m_block_size+1)*y + x;
                *dest++ = vertex_base;
                *dest++ = vertex_base + 1;
                *dest++ = vertex_base + m_block_size + 1;

                *dest++ = vertex_base + m_block_size + 1;
                *dest++ = vertex_base + 1;
                *dest++ = vertex_base + m_block_size + 1 + 1;
            }
        }
        ibo_alloc.mark_dirty();
    }

    m_allocations.emplace_back(
                std::make_tuple(std::move(ibo_alloc), std::move(vbo_alloc), world_size));
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
    }
    m_mat.sync();
    m_mat.shader().bind();
    glUniform3fv(m_mat.shader().uniform_location("lod_viewpoint"), 1, context.viewpoint().as_array);
}

void CPUFluid::render(RenderContext &context, const FullTerrainNode &fullterrain)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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
