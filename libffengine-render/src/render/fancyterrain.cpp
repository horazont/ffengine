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


namespace ffe {

static io::Logger &logger = io::logging().get_logger("render.fancyterrain");

static const Vector3f fake_viewpoint(30, 30, 200);


FancyTerrainNode::FancyTerrainNode(const unsigned int terrain_size,
                                   const unsigned int grid_size,
                                   FancyTerrainInterface &terrain_interface,
                                   GLResourceManager &resources, RenderPass &solid_pass):
    FullTerrainRenderer(terrain_size, grid_size),
    m_resources(resources),
    m_solid_pass(solid_pass),
    m_eval_context(resources.shader_library()),
    m_terrain_interface(terrain_interface),
    m_terrain(terrain_interface.terrain()),
    m_terrain_nt(terrain_interface.ntmap()),
    m_vertex_shader(resources.load_shader_checked(
                        ":/shaders/terrain/main.vert")),
    m_geometry_shader(resources.load_shader_checked(
                          ":/shaders/terrain/main.geom")),
    m_invalidate_cache_conn(terrain_interface.field_updated().connect(
                                sigc::mem_fun(*this,
                                              &FancyTerrainNode::invalidate_cache))),
    m_linear_filter(true),
    m_sharp_geometry(true),
    m_configured(false),
    m_heightmap(GL_RGB32F,
                m_terrain.size(),
                m_terrain.size(),
                GL_RGB,
                GL_FLOAT),
    m_normalt(GL_RGBA32F,
                m_terrain.size(),
                m_terrain.size(),
                GL_RGBA,
                GL_FLOAT),
    m_grass(nullptr),
    m_blend(nullptr),
    m_rock(nullptr),
    m_sand(nullptr),
    m_fluid_data(nullptr),
    m_vbo(VBOFormat({
                        VBOAttribute(2)
                    })),
    m_material(m_vbo, m_ibo),
    m_vbo_allocation(m_vbo.allocate(terrain_interface.grid_size()*terrain_interface.grid_size())),
    m_ibo_allocation(),
    m_cache_invalidation(0, 0, m_terrain.size(), m_terrain.size())
{
    const float heightmap_factor = 1.f / m_terrain_interface.size();
    m_eval_context.define1f("HEIGHTMAP_FACTOR", heightmap_factor);

    {
        auto slice = VBOSlice<Vector2f>(m_vbo_allocation, 0);
        unsigned int index = 0;
        for (unsigned int y = 0; y < grid_size; y++) {
            for (unsigned int x = 0; x < grid_size; x++) {
                slice[index++] = Vector2f(x, y) / (grid_size-1);
            }
        }
    }
    m_vbo_allocation.mark_dirty();

    m_vbo.sync();
    m_ibo.sync();
}

FancyTerrainNode::~FancyTerrainNode()
{
    m_invalidate_cache_conn.disconnect();
}

void FancyTerrainNode::configure_materials()
{
    raise_last_gl_error();
    bool success = true;

    spp::EvaluationContext main_context(m_eval_context);
    main_context.define1f("ZOFFSET", 0.f);
    if (m_fluid_data) {
        main_context.define("USE_WATER_DEPTH", "");
    }

    m_material = Material(m_vbo, m_ibo);
    {
        MaterialPass &mat = m_material.make_pass_material(m_solid_pass);
        mat.set_order(-2);

        success = success && mat.shader().attach(
                    m_vertex_shader,
                    main_context,
                    GL_VERTEX_SHADER);
        if (m_sharp_geometry) {
            success = success && mat.shader().attach(
                        m_geometry_shader,
                        main_context,
                        GL_GEOMETRY_SHADER);
        }
        success = success && mat.shader().attach(
                    m_resources.load_shader_checked(":/shaders/terrain/main.frag"),
                    main_context,
                    GL_FRAGMENT_SHADER);
    }
    m_material.declare_attribute("position", 0);

    success = success && m_material.link();

    if (!success) {
        throw std::runtime_error("failed to compile or link terrain material");
    }

    m_material.attach_texture("heightmap", &m_heightmap);
    m_material.attach_texture("normalt", &m_normalt);
    if (m_blend) {
        m_material.attach_texture("blend", m_blend);
    }
    if (m_grass) {
        m_material.attach_texture("grass", m_grass);
    }
    if (m_rock) {
        m_material.attach_texture("rock", m_rock);
    }
    if (m_sand) {
        m_material.attach_texture("sand", m_sand);
    }
    if (m_fluid_data) {
        m_material.attach_texture("fluid_data", m_fluid_data);
    }
    {
        MaterialPass &mat = m_material.make_pass_material(m_solid_pass);
        mat.shader().bind();
        glUniform2f(mat.shader().uniform_location("chunk_translation"),
                    0.0, 0.0);

    }

    raise_last_gl_error();

    for (auto &entry: m_overlays)
    {
        configure_single_overlay_material(*entry.first, entry.second);
        raise_last_gl_error();
    }

    raise_last_gl_error();
}

void FancyTerrainNode::configure_single_overlay_material(
        const spp::Program &fragment_shader,
        OverlayConfig &config)
{
    spp::EvaluationContext overlay_context(m_eval_context);
    overlay_context.define1f("ZOFFSET", 1.f);

    config.material = std::make_unique<Material>(m_vbo, m_ibo);
    MaterialPass &pass = config.material->make_pass_material(m_solid_pass);
    pass.set_order(-1);

    bool success = pass.shader().attach(
                m_vertex_shader,
                overlay_context,
                GL_VERTEX_SHADER);
    if (m_sharp_geometry) {
        success = success && pass.shader().attach(
                    m_geometry_shader,
                    overlay_context,
                    GL_GEOMETRY_SHADER);
    }
    success = success && pass.shader().attach(
                fragment_shader,
                overlay_context,
                GL_FRAGMENT_SHADER);

    config.material->declare_attribute("position", 0);

    success = success && config.material->link();

    if (!success) {
        throw std::runtime_error("failed to compile or link overlay material");
    }

    if (config.configure_callback) {
        config.configure_callback(pass);
    }

    config.material->attach_texture("heightmap", &m_heightmap);
    config.material->attach_texture("normalt", &m_normalt);

    config.material->set_depth_mask(false);
}

void FancyTerrainNode::configure_without_sharp_geometry()
{
    m_ibo_allocation = m_ibo.allocate((m_terrain_interface.grid_size()-1)*(m_terrain_interface.grid_size()-1)*6);
    uint16_t *dest = m_ibo_allocation.get();
    for (unsigned int y = 0; y < m_grid_size-1; y++) {
        for (unsigned int x = 0; x < m_grid_size-1; x++) {
            const unsigned int curr_base = y*m_grid_size + x;
            *dest++ = curr_base + m_grid_size;
            *dest++ = curr_base;
            *dest++ = curr_base + m_grid_size + 1;

            *dest++ = curr_base + m_grid_size + 1;
            *dest++ = curr_base;
            *dest++ = curr_base + 1;
        }
    }
    m_ibo_allocation.mark_dirty();
}

void FancyTerrainNode::configure_with_sharp_geometry()
{
    m_ibo_allocation = m_ibo.allocate((m_terrain_interface.grid_size()-1)*(m_terrain_interface.grid_size()-1)*4);
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
}

void FancyTerrainNode::reconfigure()
{
    if (m_configured) {
        return;
    }
    m_configured = true;

    m_ibo_allocation = nullptr;
    if (m_sharp_geometry) {
        configure_with_sharp_geometry();
    } else {
        configure_without_sharp_geometry();
    }

    configure_materials();

    m_vbo.sync();
    m_ibo.sync();
}

inline void render_slice(RenderContext &context,
                         Material &material,
                         IBOAllocation &ibo_allocation,
                         VBOAllocation &vbo_allocation,
                         const float x, const float y,
                         const float scale,
                         const GLenum mode,
                         const float data_layer)
{
    /*const float xtex = (float(slot_index % texture_cache_size) + 0.5/grid_size) / texture_cache_size;
    const float ytex = (float(slot_index / texture_cache_size) + 0.5/grid_size) / texture_cache_size;*/

    /*std::cout << "rendering" << std::endl;
    std::cout << "  translationx  = " << x << std::endl;
    std::cout << "  translationy  = " << y << std::endl;
    std::cout << "  scale         = " << scale << std::endl;*/
    context.render_all(AABB{}, mode, material,
                       ibo_allocation, vbo_allocation,
                       [scale, x, y, data_layer](MaterialPass &pass){
                           glUniform1f(pass.shader().uniform_location("chunk_size"), scale);
                           glUniform2f(pass.shader().uniform_location("chunk_translation"), x, y);
                           glUniform1f(pass.shader().uniform_location("data_layer"), data_layer);
                       });
}

void FancyTerrainNode::render_all(RenderContext &context, Material &material,
                                  const FullTerrainNode &parent,
                                  const FullTerrainNode::Slices &slices_to_render)
{
    const GLenum mode = (m_sharp_geometry ? GL_LINES_ADJACENCY : GL_TRIANGLES);
    for (auto &slice: slices_to_render) {
        const float x = slice.basex;
        const float y = slice.basey;
        const float scale = slice.lod;

        render_slice(context, material, m_ibo_allocation, m_vbo_allocation,
                     x, y, scale,
                     mode,
                     parent.get_texture_layer_for_slice(slice).first);
    }
}

void FancyTerrainNode::sync_material(Material &material,
                                     const float scale_to_radius)
{
    for (auto iter = material.cbegin();
         iter != material.cend();
         ++iter)
    {
        MaterialPass &pass = *iter->second;
        pass.shader().bind();
        glUniform1f(pass.shader().uniform_location("scale_to_radius"),
                    scale_to_radius);
    }
}

void FancyTerrainNode::update_material(RenderContext &context, Material &material)
{
    for (auto iter = material.cbegin();
         iter != material.cend();
         ++iter)
    {
        MaterialPass &pass = *iter->second;
        pass.shader().bind();
        glUniform3fv(pass.shader().uniform_location("lod_viewpoint"),
                     1, context.viewpoint()/*fake_viewpoint*/.as_array);
    }
}

void FancyTerrainNode::attach_blend_texture(Texture2D *tex)
{
    if (m_configured) {
        m_material.attach_texture("blend", tex);
    }
    m_blend = tex;
}

void FancyTerrainNode::attach_grass_texture(Texture2D *tex)
{
    if (m_configured) {
        m_material.attach_texture("grass", tex);
    }
    m_grass = tex;
}

void FancyTerrainNode::attach_rock_texture(Texture2D *tex)
{
    if (m_configured) {
        m_material.attach_texture("rock", tex);
    }
    m_rock = tex;
}

void FancyTerrainNode::attach_sand_texture(Texture2D *tex)
{
    if (m_configured) {
        m_material.attach_texture("sand", tex);
    }
    m_sand = tex;
}

void FancyTerrainNode::attach_fluid_data_texture(Texture2DArray *tex)
{
    if (m_fluid_data != tex) {
        m_configured = false;
    }
    m_fluid_data = tex;
}

void FancyTerrainNode::reposition_overlay(
        const spp::Program &fragment_shader,
        const sim::TerrainRect &clip_rect)
{
    OverlayConfig &conf = m_overlays.at(&fragment_shader);
    conf.clip_rect = clip_rect;
}

void FancyTerrainNode::configure_overlay_material(
        const spp::Program &fragment_shader,
        std::function<void (MaterialPass &)> &&configure_callback)
{
    OverlayConfig &conf = m_overlays[&fragment_shader];
    conf.material = nullptr;
    conf.configure_callback = std::move(configure_callback);
    if (m_configured) {
        configure_single_overlay_material(fragment_shader, conf);
    }
}

MaterialPass *FancyTerrainNode::get_overlay_material(const spp::Program &fragment_shader)
{
    auto iter = m_overlays.find(&fragment_shader);
    if (iter == m_overlays.end()) {
        return nullptr;
    }

    reconfigure();
    return iter->second.material->pass_material(m_solid_pass);
}

void FancyTerrainNode::remove_overlay(const spp::Program &fragment_shader)
{
    auto iter = m_overlays.find(&fragment_shader);
    if (iter == m_overlays.end()) {
        return;
    }

    m_overlays.erase(iter);
}

void FancyTerrainNode::set_sharp_geometry(bool use)
{
    if (m_sharp_geometry == use) {
        return;
    }
    m_sharp_geometry = use;
    m_configured = false;
}

void FancyTerrainNode::invalidate_cache(sim::TerrainRect part)
{
    logger.log(io::LOG_INFO, "GPU terrain data cache invalidated");
    std::lock_guard<std::mutex> lock(m_cache_invalidation_mutex);
    m_cache_invalidation = bounds(part, m_cache_invalidation);
}

void FancyTerrainNode::render(RenderContext &context,
                              const FullTerrainNode &parent,
                              const FullTerrainNode::Slices &slices)
{
    update_material(context, m_material);
    render_all(context, m_material, parent, slices);

    const GLenum mode = (m_sharp_geometry ? GL_LINES_ADJACENCY : GL_TRIANGLES);
    for (auto &overlay: m_render_overlays)
    {
        Material &material = *overlay.material;
        update_material(context, material);
        for (auto &slice: slices)
        {
            const unsigned int x = slice.basex;
            const unsigned int y = slice.basey;
            const unsigned int scale = slice.lod;

            sim::TerrainRect slice_rect(x, y, x+scale, y+scale);
            if (slice_rect.overlaps(overlay.clip_rect)) {
                render_slice(context,
                             material, m_ibo_allocation, m_vbo_allocation,
                             x, y, scale,
                             mode,
                             parent.get_texture_layer_for_slice(slice).first);
            }
        }
    }
}

void FancyTerrainNode::sync(const FullTerrainNode &fullterrain)
{
    reconfigure();

    m_render_overlays.clear();
    m_render_overlays.reserve(m_overlays.size());
    for (auto &item: m_overlays) {
        if (item.second.clip_rect == NotARect) {
            continue;
        }

        sync_material(*item.second.material,
                      fullterrain.scale_to_radius());
        m_render_overlays.emplace_back(
                    RenderOverlay{item.second.material.get(), item.second.clip_rect}
                    );
    }

    sync_material(m_material,
                  fullterrain.scale_to_radius());

    m_heightmap.bind();
    if (m_linear_filter) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    m_normalt.bind();
    if (m_linear_filter) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

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
            const sim::Terrain::Field *heightfield = nullptr;
            auto hf_lock = m_terrain.readonly_field(heightfield);

            glTexSubImage2D(GL_TEXTURE_2D, 0,
                            updated.x0(),
                            updated.y0(),
                            updated.x1() - updated.x0(),
                            updated.y1() - updated.y0(),
                            GL_RGB, GL_FLOAT,
                            &heightfield->data()[updated.y0()*m_terrain.size()+updated.x0()].as_array);

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
}

void FancyTerrainNode::prepare(RenderContext &,
                               const FullTerrainNode &,
                               const FullTerrainNode::Slices &)
{

}

}
