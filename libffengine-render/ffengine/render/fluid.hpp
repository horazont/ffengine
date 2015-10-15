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
#include "ffengine/render/renderpass.hpp"


namespace sim {

class WorldState;
class SignalQueue;

}


namespace ffe {


struct FluidSlice
{
    FluidSlice(IBOAllocation &&ibo_alloc, VBOAllocation &&vbo_alloc,
               unsigned int size,
               std::basic_string<Vector4f> &&data_texture,
               std::basic_string<Vector4f> &&normalt_texture);

    IBOAllocation m_ibo_alloc;
    VBOAllocation m_vbo_alloc;
    unsigned int m_size;
    unsigned int m_layer;
    float m_base_x, m_base_y;
    std::basic_string<Vector4f> m_data_texture;
    std::basic_string<Vector4f> m_normalt_texture;

    /**
     * The usage level describes how the fluid slice was used in the last frame.
     *
     * It is incremeted for each prepare() call which makes use of the slice.
     * sync() will evict slices in each frame until a configurable threshold is
     * reached. For this it will sort the slices by their usage level and start
     * deleting them starting with the lowest usage level.
     */
    unsigned int m_usage_level;
};


class CPUFluid: public FullTerrainRenderer
{
public:
    CPUFluid(const unsigned int terrain_size,
             const unsigned int grid_size,
             GLResourceManager &resources,
             const sim::WorldState &state,
             sim::SignalQueue &signal_queue,
             RenderPass &transparent_pass,
             RenderPass &water_pass);

private:
    RenderPass &m_transparent_pass;
    RenderPass &m_water_pass;
    GLResourceManager &m_resources;
    const sim::Fluid &m_fluidsim;
    const unsigned int m_block_size;
    const unsigned int m_lods;

    sig11::connection_guard<void()> m_fluid_resetted_guard;

    unsigned int m_max_slices;

    Material m_mat;
    Texture2DArray m_fluid_data;
    Texture2DArray m_normalt;

    std::vector<std::vector<std::pair<bool, std::unique_ptr<FluidSlice> > > > m_slice_cache;
    std::unordered_map<RenderContext*, std::vector<FluidSlice*> > m_render_slices;

    std::vector<const sim::FluidBlock*> m_tmp_used_blocks;
    std::vector<Vector4f> m_tmp_fluid_data_cache;
    std::vector<unsigned int> m_tmp_index_mapping;
    std::vector<std::tuple<Vector3f, Vector4f> > m_tmp_vertex_data;
    std::vector<uint16_t> m_tmp_index_data;

    typedef std::basic_string<Vector4f> FluidDataTextureBuffer;
    typedef std::basic_string<Vector4f> NormalTTextureBuffer;
    FluidDataTextureBuffer m_tmp_data_texture;
    NormalTTextureBuffer m_tmp_normalt_texture;

    typedef std::tuple<unsigned int, unsigned int, unsigned int> CacheTuple;
    std::vector<CacheTuple> m_tmp_slices;

private:
    void fluid_resetted();
    void reinitialise_cache();

    void invalidate_caches(const unsigned int blockx,
                           const unsigned int blocky);


    unsigned int request_vertex_inject(const float x0f,
                                       const float y0f,
                                       const unsigned int oversample,
                                       const unsigned int x,
                                       const unsigned int y);

    std::unique_ptr<FluidSlice> produce_geometry(
            const unsigned int blockx,
            const unsigned int blocky,
            const unsigned int world_size,
            const unsigned int oversample);

public:
    inline Texture2DArray *fluid_data()
    {
        return &m_fluid_data;
    }

public:
    void prepare(RenderContext &context,
                 const FullTerrainNode &fullterrain,
                 const FullTerrainNode::Slices &slices) override;
    void render(RenderContext &context,
                const FullTerrainNode &fullterrain,
                const FullTerrainNode::Slices &slices) override;
    void sync(const FullTerrainNode &fullterrain) override;

};


}

#endif
