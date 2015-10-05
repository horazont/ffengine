/**********************************************************************
File name: fullterrain.cpp
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
#include "ffengine/render/fullterrain.hpp"

#include "ffengine/math/algo.hpp"
#include "ffengine/math/intersect.hpp"


namespace ffe {

/* engine::TerrainSlice */

TerrainSlice::TerrainSlice():
    basex(0),
    basey(0),
    lod(0),
    valid(false)
{

}

TerrainSlice::TerrainSlice(unsigned int basex,
                           unsigned int basey,
                           unsigned int lod):
    basex(basex),
    basey(basey),
    lod(lod),
    valid(true)
{

}

/* engine::FullTerrainRenderer */

FullTerrainRenderer::FullTerrainRenderer(const unsigned int terrain_size,
                                         const unsigned int grid_size):
    m_terrain_size(terrain_size),
    m_grid_size(grid_size)
{

}

FullTerrainRenderer::~FullTerrainRenderer()
{

}


/* engine::FullTerrainNode::SliceBookkeeping */

FullTerrainNode::SliceBookkeeping::SliceBookkeeping():
    m_texture_layer(-1),
    m_usage_level(-1),
    m_invalidated(true)
{

}

FullTerrainNode::SliceBookkeeping::SliceBookkeeping(int texture_layer,
                                                    int usage_level):
    m_texture_layer(texture_layer),
    m_usage_level(usage_level),
    m_invalidated(true)
{

}


/* engine::FullTerrainNode */

FullTerrainNode::FullTerrainNode(const unsigned int terrain_size,
                                 const unsigned int grid_size):
    m_terrain_size(terrain_size),
    m_grid_size(grid_size),
    m_max_depth(log2_of_pot((m_terrain_size-1)/(m_grid_size-1))),
    m_detail_level((unsigned int)-1)
{
    m_layer_slices.resize(512);
    set_detail_level(1);
}

int FullTerrainNode::acquire_layer_for_slice(const TerrainSlice &slice)
{
    if (!slice) {
        return -1;
    }

    auto iter = m_slice_bookkeeping.find(slice);
    if (iter != m_slice_bookkeeping.end()) {
        return iter->second.m_texture_layer;
    }

    for (unsigned int i = 0; i < m_layer_slices.size(); ++i) {
        if (!m_layer_slices[i]) {
            m_slice_bookkeeping[slice] = SliceBookkeeping(i, 0);
            m_layer_slices[i] = slice;
            return i;
        }
    }

    return -1;
}

void FullTerrainNode::collect_slices_recurse(
        Slices &dest,
        const unsigned int invdepth,
        const unsigned int relative_x,
        const unsigned int relative_y,
        const Vector3f &viewpoint,
        const std::array<Plane, 6> &frustum)
{
    const float min = 0.;
    const float max = 0.;

    const unsigned int size = (1u << invdepth)*(m_grid_size-1);

    const unsigned int absolute_x = relative_x * size;
    const unsigned int absolute_y = relative_y * size;

    AABB box{Vector3f(absolute_x, absolute_y, min),
             Vector3f(absolute_x+size, absolute_y+size, max)};

    PlaneSide side = isect_aabb_frustum(box, frustum);
    if (side == PlaneSide::NEGATIVE_NORMAL) {
        // outside frustum
        return;
    }

    const float next_range_radius = m_lod_range_base * (1u<<invdepth);
    if (invdepth == 0 ||
            !isect_aabb_sphere(box, Sphere{viewpoint, next_range_radius}))
    {
        // next LOD not required, insert node
        dest.emplace_back(absolute_x, absolute_y, size);
        touch_slice(dest.back());
        return;
    }

    // some children will need higher LOD

    for (unsigned int offsy = 0; offsy < 2; offsy++) {
        for (unsigned int offsx = 0; offsx < 2; offsx++) {
            collect_slices_recurse(
                        dest,
                        invdepth-1,
                        relative_x*2+offsx,
                        relative_y*2+offsy,
                        viewpoint,
                        frustum);
        }
    }
}

void FullTerrainNode::touch_slice(const TerrainSlice &slice)
{
    if (!slice) {
        return;
    }

    auto iter = m_slice_bookkeeping.find(slice);
    if (iter == m_slice_bookkeeping.end()) {
        if (acquire_layer_for_slice(slice) >= 0) {
            iter = m_slice_bookkeeping.find(slice);
            assert(iter != m_slice_bookkeeping.end());
        } else {
            return;
        }
    }
    iter->second.m_usage_level += 1;
}

int FullTerrainNode::get_texture_layer_for_slice(
        const TerrainSlice &slice) const
{
    auto iter = m_slice_bookkeeping.find(slice);
    if (iter == m_slice_bookkeeping.end()) {
        return -1;
    }
    return iter->second.m_texture_layer;
}

void FullTerrainNode::set_detail_level(unsigned int level)
{
    if (level >= m_max_depth) {
        level = m_max_depth;
    }

    m_detail_level = level;
    m_lod_range_base = (m_grid_size << level) - 1;
}

void FullTerrainNode::prepare(RenderContext &context)
{
    collect_slices_recurse(
                m_render_slices[&context],
                m_max_depth, 0, 0,
                context.viewpoint()/*fake_viewpoint*/,
                context.frustum());

    for (auto &renderer: m_renderers) {
        renderer->prepare(context, *this, m_render_slices[&context]);
    }
}

void FullTerrainNode::render(RenderContext &context)
{
    for (auto &renderer: m_renderers) {
        renderer->render(context, *this, m_render_slices[&context]);
    }
}

void FullTerrainNode::sync()
{
    for (auto &slice: m_layer_slices) {
        slice = TerrainSlice();
    }
    m_slice_bookkeeping.clear();

    m_render_slices.clear();
    for (auto &renderer: m_renderers) {
        renderer->sync(*this);
    }
}

}
