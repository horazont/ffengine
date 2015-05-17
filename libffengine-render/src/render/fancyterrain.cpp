/**********************************************************************
File name: fancyterrain.cpp
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
#include "ffengine/render/fancyterrain.hpp"

#include "ffengine/common/utils.hpp"
#include "ffengine/math/intersect.hpp"
#include "ffengine/io/log.hpp"

// #define TIMELOG_SYNC
// #define TIMELOG_RENDER

#if defined(TIMELOG_SYNC) || defined(TIMELOG_RENDER)
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;
#define ms_cast(x) std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(x)
#endif

namespace engine {

static io::Logger &logger = io::logging().get_logger("render.fancyterrain");

static const Vector3f fake_viewpoint(30, 30, 200);


HeightmapSliceMeta::HeightmapSliceMeta():
    valid(false)
{

}

HeightmapSliceMeta::HeightmapSliceMeta(
        unsigned int basex,
        unsigned int basey,
        unsigned int lod):
    basex(basex),
    basey(basey),
    lod(lod),
    valid(true)
{

}


FancyTerrainNode::FancyTerrainNode(FancyTerrainInterface &terrain_interface):
    m_terrain_interface(terrain_interface),
    m_grid_size(terrain_interface.grid_size()),
    m_tiles((terrain_interface.size()-1)/(terrain_interface.grid_size()-1)),
    m_max_depth(log2_of_pot((terrain_interface.size()-1)/(m_grid_size-1))),
    m_terrain(terrain_interface.terrain()),
    m_terrain_nt(terrain_interface.ntmap()),
    m_invalidate_cache_conn(terrain_interface.field_updated().connect(
                           sigc::mem_fun(*this,
                                         &FancyTerrainNode::invalidate_cache))),
    m_heightmap(GL_R32F,
                m_terrain.size(),
                m_terrain.size(),
                GL_RED,
                GL_FLOAT),
    m_normalt(GL_RGBA32F,
                m_terrain.size(),
                m_terrain.size(),
                GL_RGBA,
                GL_FLOAT),
    m_vbo(VBOFormat({
                        VBOAttribute(2)
                    })),
    m_ibo(),
    m_vbo_allocation(m_vbo.allocate(m_grid_size*m_grid_size)),
    m_ibo_allocation(m_ibo.allocate((m_grid_size-1)*(m_grid_size-1)*4)),
    m_cache_invalidation(0, 0, m_terrain.size(), m_terrain.size())
{
    uint16_t *dest = m_ibo_allocation.get();
    for (unsigned int y = 0; y < m_grid_size-1; y++) {
        for (unsigned int x = 0; x < m_grid_size-1; x++) {
            const unsigned int curr_base = y*m_grid_size + x;
            *dest++ = curr_base;
            *dest++ = curr_base + m_grid_size;
            *dest++ = curr_base + m_grid_size + 1;
            *dest++ = curr_base + 1;
        }
    }

    m_ibo_allocation.mark_dirty();

    bool success = true;

    success = success && m_material.shader().attach_resource(
                GL_VERTEX_SHADER,
                ":/shaders/terrain/main.vert");
    success = success && m_material.shader().attach_resource(
                GL_GEOMETRY_SHADER,
                ":/shaders/terrain/main.geom");
    success = success && m_material.shader().attach_resource(
                GL_FRAGMENT_SHADER,
                ":/shaders/terrain/main.frag");
    success = success && m_material.shader().link();

    success = success && m_normal_debug_material.shader().attach_resource(
                GL_VERTEX_SHADER,
                ":/shaders/terrain/main.vert");
    success = success && m_normal_debug_material.shader().attach_resource(
                GL_GEOMETRY_SHADER,
                ":/shaders/terrain/normal_debug.geom");
    success = success && m_normal_debug_material.shader().attach_resource(
                GL_FRAGMENT_SHADER,
                ":/shaders/generic/normal_debug.frag");
    success = success && m_normal_debug_material.shader().link();

    if (!success) {
        throw std::runtime_error("failed to compile or link shader");
    }

    const float heightmap_factor = 1.f / m_terrain_interface.size();

    m_material.shader().bind();
    glUniform2f(m_material.shader().uniform_location("chunk_translation"),
                0.0, 0.0);
    m_material.attach_texture("heightmap", &m_heightmap);
    m_material.attach_texture("normalt", &m_normalt);
    glUniform1f(m_material.shader().uniform_location("zoffset"),
                0.);
    glUniform1f(m_material.shader().uniform_location("heightmap_factor"),
                heightmap_factor);
    RenderContext::configure_shader(m_material.shader());

    m_normal_debug_material.shader().bind();
    glUniform2f(m_normal_debug_material.shader().uniform_location("chunk_translation"),
                0.0, 0.0);
    m_normal_debug_material.attach_texture("heightmap", &m_heightmap);
    m_normal_debug_material.attach_texture("normalt", &m_normalt);
    glUniform1f(m_normal_debug_material.shader().uniform_location("normal_length"),
                2.);
    glUniform1f(m_normal_debug_material.shader().uniform_location("zoffset"),
                0.);
    glUniform1f(m_normal_debug_material.shader().uniform_location("heightmap_factor"),
                heightmap_factor);
    RenderContext::configure_shader(m_normal_debug_material.shader());

    m_heightmap.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/

    m_normalt.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);*/

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);
    m_nd_vao = decl.make_vao(m_normal_debug_material.shader(), true);

    {
        auto slice = VBOSlice<Vector2f>(m_vbo_allocation, 0);
        unsigned int index = 0;
        for (unsigned int y = 0; y < m_grid_size; y++) {
            for (unsigned int x = 0; x < m_grid_size; x++) {
                slice[index++] = Vector2f(x, y) / (m_grid_size-1);
            }
        }
    }
    m_vbo_allocation.mark_dirty();

    m_vao->sync();
    m_nd_vao->sync();

    // we assume that at any given time only one fourth of the texture cache
    // will be used for rendering. otherwise, realloc :)
    m_render_slices.reserve(m_tiles / 2);
}

FancyTerrainNode::~FancyTerrainNode()
{
    m_invalidate_cache_conn.disconnect();
}

void FancyTerrainNode::collect_slices(
        std::vector<HeightmapSliceMeta> &requested_slices,
        const std::array<Plane, 4> &frustum,
        const Vector3f &viewpoint)
{
    collect_slices_recurse(requested_slices,
                           m_max_depth,
                           0,
                           0,
                           viewpoint,
                           frustum);
}

void FancyTerrainNode::collect_slices_recurse(
        std::vector<HeightmapSliceMeta> &requested_slices,
        const unsigned int invdepth,
        const unsigned int relative_x,
        const unsigned int relative_y,
        const Vector3f &viewpoint,
        const std::array<Plane, 4> &frustum)
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

    const float next_range_radius = lod_range_base * (1u<<invdepth);
    if (invdepth == 0 ||
            !isect_aabb_sphere(box, Sphere{viewpoint, next_range_radius}))
    {
        // next LOD not required, insert node
        requested_slices.push_back(
                    HeightmapSliceMeta(absolute_x, absolute_y, size)
                    );
        return;
    }

    // some children will need higher LOD

    for (unsigned int offsy = 0; offsy < 2; offsy++) {
        for (unsigned int offsx = 0; offsx < 2; offsx++) {
            collect_slices_recurse(
                        requested_slices,
                        invdepth-1,
                        relative_x*2+offsx,
                        relative_y*2+offsy,
                        viewpoint,
                        frustum);
        }
    }
}

inline void render_slice(RenderContext &context,
                         VAO &vao,
                         Material &material,
                         IBOAllocation &ibo_allocation,
                         const float x, const float y,
                         const float scale)
{
    /*const float xtex = (float(slot_index % texture_cache_size) + 0.5/grid_size) / texture_cache_size;
    const float ytex = (float(slot_index / texture_cache_size) + 0.5/grid_size) / texture_cache_size;*/

    glUniform1f(material.shader().uniform_location("chunk_size"),
                scale);
    glUniform2f(material.shader().uniform_location("chunk_translation"),
                x, y);
    /*std::cout << "rendering" << std::endl;
    std::cout << "  translationx  = " << x << std::endl;
    std::cout << "  translationy  = " << y << std::endl;
    std::cout << "  scale         = " << scale << std::endl;*/
    context.draw_elements(GL_LINES_ADJACENCY, vao, material, ibo_allocation);
}

void FancyTerrainNode::render_all(RenderContext &context, VAO &vao, Material &material)
{
    for (auto &slice: m_render_slices) {
        const float x = slice.basex;
        const float y = slice.basey;
        const float scale = slice.lod;

        render_slice(context, vao, material, m_ibo_allocation,
                     x, y, scale);
    }
}

void FancyTerrainNode::attach_blend_texture(Texture2D *tex)
{
    m_material.attach_texture("blend", tex);
}

void FancyTerrainNode::attach_grass_texture(Texture2D *tex)
{
    m_material.attach_texture("grass", tex);
}

void FancyTerrainNode::attach_rock_texture(Texture2D *tex)
{
    m_material.attach_texture("rock", tex);
}

void FancyTerrainNode::configure_overlay(
        Material &mat,
        const sim::TerrainRect &clip_rect)
{
    OverlayConfig &conf = m_overlays[&mat];
    conf.clip_rect = clip_rect;
}

bool FancyTerrainNode::configure_overlay_material(Material &mat)
{
    bool success = mat.shader().attach_resource(GL_VERTEX_SHADER,
                                                ":/shaders/terrain/main.vert");
    success = success && mat.shader().attach_resource(GL_GEOMETRY_SHADER,
                                                ":/shaders/terrain/main.geom");
    success = success && mat.shader().link();
    if (!success) {
        return false;
    }

    mat.attach_texture("heightmap", &m_heightmap);
    mat.attach_texture("normalt", &m_normalt);
    mat.shader().bind();
    glUniform1f(mat.shader().uniform_location("zoffset"), 1.0f);
    glUniform1f(mat.shader().uniform_location("heightmap_factor"),
                1.f/(m_terrain.size()));

    return true;
}

void FancyTerrainNode::remove_overlay(Material &mat)
{
    auto iter = m_overlays.find(&mat);
    if (iter == m_overlays.end()) {
        return;
    }

    m_overlays.erase(iter);
}

void FancyTerrainNode::invalidate_cache(sim::TerrainRect part)
{
    logger.log(io::LOG_INFO, "GPU terrain data cache invalidated");
    std::lock_guard<std::mutex> lock(m_cache_invalidation_mutex);
    m_cache_invalidation = bounds(part, m_cache_invalidation);
}

void FancyTerrainNode::render(RenderContext &context)
{
#ifdef TIMELOG_RENDER
    const timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_solid, t_overlay;
#endif
    /* glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); */
    m_material.shader().bind();
    glUniform3fv(m_material.shader().uniform_location("lod_viewpoint"),
                 1, context.viewpoint()/*fake_viewpoint*/.as_array);
    render_all(context, *m_vao, m_material);
    /* glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); */
#ifdef TIMELOG_RENDER
    t_solid = timelog_clock::now();
#endif

    /* m_normal_debug_material.shader().bind();
    glUniform3fv(m_normal_debug_material.shader().uniform_location("lod_viewpoint"),
                 1, context.scene().viewpoint().as_array);
    render_all(context, *m_nd_vao, m_normal_debug_material); */

    glDepthMask(GL_FALSE);
    for (auto &overlay: m_render_overlays)
    {
        overlay.material->shader().bind();
        glUniform3fv(overlay.material->shader().uniform_location("lod_viewpoint"),
                     1, context.viewpoint()/*fake_viewpoint*/.as_array);
        for (auto &slice: m_render_slices)
        {
            const unsigned int x = slice.basex;
            const unsigned int y = slice.basey;
            const unsigned int scale = slice.lod;

            sim::TerrainRect slice_rect(x, y, x+scale, y+scale);
            if (slice_rect.overlaps(overlay.clip_rect)) {
                render_slice(context,
                             *m_vao, *overlay.material, m_ibo_allocation,
                             x, y, scale);
            }
        }
    }
    glDepthMask(GL_TRUE);
#ifdef TIMELOG_RENDER
    t_overlay = timelog_clock::now();
    logger.logf(io::LOG_DEBUG, "render: solid time: %.2f ms",
                std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(t_solid-t0).count());
    logger.logf(io::LOG_DEBUG, "render: overlay time: %.2f ms",
                std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(t_overlay-t_solid).count());
#endif
}

void FancyTerrainNode::sync(RenderContext &context)
{
    // FIXME: use SceneStorage here!

#ifdef TIMELOG_SYNC
    const timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_overlays, t_setup, t_allocate, t_upload;
#endif

    m_render_overlays.clear();
    m_render_overlays.reserve(m_overlays.size());
    for (auto &item: m_overlays) {
        item.first->bind();
        glUniform1f(item.first->shader().uniform_location("scale_to_radius"),
                    lod_range_base / (m_grid_size-1));
        m_render_overlays.emplace_back(
                    RenderOverlay{item.first, item.second.clip_rect}
                    );
    }

#ifdef TIMELOG_SYNC
    t_overlays = timelog_clock::now();
#endif

    m_render_slices.clear();
    collect_slices(m_render_slices, context.frustum(), context.viewpoint()/*fake_viewpoint*/);

    m_material.shader().bind();
    glUniform1f(m_material.shader().uniform_location("scale_to_radius"),
                lod_range_base / (m_grid_size-1));
    m_normal_debug_material.shader().bind();
    glUniform1f(m_normal_debug_material.shader().uniform_location("scale_to_radius"),
                lod_range_base / (m_grid_size-1));

#ifdef TIMELOG_SYNC
    t_setup = timelog_clock::now();
#endif

#ifdef TIMELOG_SYNC
    t_allocate = timelog_clock::now();
#endif

    sim::TerrainRect updated;
    {
        std::lock_guard<std::mutex> lock(m_cache_invalidation_mutex);
        updated = m_cache_invalidation;
        m_cache_invalidation = NotARect;
    }
    if (updated.is_a_rect())
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, m_terrain.size());
        m_heightmap.bind();
        {
            const sim::Terrain::HeightField *heightfield = nullptr;
            auto hf_lock = m_terrain.readonly_field(heightfield);

            glTexSubImage2D(GL_TEXTURE_2D, 0,
                            updated.x0(),
                            updated.y0(),
                            updated.x1() - updated.x0(),
                            updated.y1() - updated.y0(),
                            GL_RED, GL_FLOAT,
                            &heightfield->data()[updated.y0()*m_terrain.size()+updated.x0()]);

        }

        m_normalt.bind();
        {
            const NTMapGenerator::NTField *ntfield = nullptr;
            auto nt_lock = m_terrain_nt.readonly_field(ntfield);

            glTexSubImage2D(GL_TEXTURE_2D, 0,
                            updated.x0(),
                            updated.y0(),
                            updated.x1() - updated.x0(),
                            updated.y1() - updated.y0(),
                            GL_RGBA, GL_FLOAT,
                            &ntfield->data()[updated.y0()*m_terrain.size()+updated.x0()]);
        }
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

#ifdef TIMELOG_SYNC
    t_upload = timelog_clock::now();
    logger.logf(io::LOG_DEBUG, "sync: t_overlays = %.2f ms", ms_cast(t_overlays - t0).count());
    logger.logf(io::LOG_DEBUG, "sync: t_setup    = %.2f ms", ms_cast(t_setup - t_overlays).count());
    logger.logf(io::LOG_DEBUG, "sync: t_allocate = %.2f ms", ms_cast(t_allocate - t_setup).count());
    logger.logf(io::LOG_DEBUG, "sync: t_upload   = %.2f ms", ms_cast(t_upload - t_allocate).count());
#endif
}


const FancyTerrainNode::SlotUsage FancyTerrainNode::SLOT_EVICTABLE = 0;
const FancyTerrainNode::SlotUsage FancyTerrainNode::SLOT_SUGGESTED = 1;
const FancyTerrainNode::SlotUsage FancyTerrainNode::SLOT_REQUIRED = 2;

}
