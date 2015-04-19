#include "engine/render/fancyterrain.hpp"

#include "engine/common/utils.hpp"
#include "engine/math/intersect.hpp"
#include "engine/io/log.hpp"

#define TIMELOG_SYNC
#define TIMELOG_RENDER

#if defined(TIMELOG_SYNC) || defined(TIMELOG_RENDER)
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;
#define ms_cast(x) std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(x)
#endif

namespace engine {

static io::Logger &logger = io::logging().get_logger("render.fancyterrain");


bool HeightmapSliceMeta::operator<(const HeightmapSliceMeta &other) const
{
    return lod < other.lod;
}


FancyTerrainNode::FancyTerrainNode(FancyTerrainInterface &terrain_interface,
                                   const unsigned int texture_cache_size):
    m_terrain_interface(terrain_interface),
    m_grid_size(terrain_interface.grid_size()),
    m_texture_cache_size(texture_cache_size),
    m_min_lod(m_grid_size-1),
    m_max_depth(log2_of_pot(terrain_interface.size()-1)),
    m_terrain(terrain_interface.terrain()),
    m_terrain_lods(terrain_interface.heightmap_lods()),
    m_terrain_minmax(terrain_interface.heightmap_minmax()),
    m_terrain_nt(terrain_interface.ntmap()),
    m_terrain_nt_lods(terrain_interface.ntmap_lods()),
    m_heightmap(GL_R32F,
                texture_cache_size*m_grid_size,
                texture_cache_size*m_grid_size,
                GL_RED,
                GL_FLOAT),
    m_normalt(GL_RGBA32F,
                texture_cache_size*m_grid_size,
                texture_cache_size*m_grid_size,
                GL_RGBA,
                GL_FLOAT),
    m_vbo(VBOFormat({
                        VBOAttribute(2)
                    })),
    m_ibo(),
    m_vbo_allocation(m_vbo.allocate(m_grid_size*m_grid_size)),
    m_ibo_allocation(m_ibo.allocate((m_grid_size-1)*(m_grid_size-1)*6))
{
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

    bool success = true;

    success = success && m_material.shader().attach_resource(
                GL_VERTEX_SHADER,
                ":/shaders/terrain/main.vert");
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

    m_material.shader().bind();
    glUniform2f(m_material.shader().uniform_location("chunk_translation"),
                0.0, 0.0);
    glUniform2f(m_material.shader().uniform_location("heightmap_base"),
                0.0, 0.0);
    m_material.attach_texture("heightmap", &m_heightmap);
    m_material.attach_texture("normalt", &m_normalt);
    glUniform1f(m_material.shader().uniform_location("zoffset"),
                0.);
    RenderContext::configure_shader(m_material.shader());

    m_normal_debug_material.shader().bind();
    glUniform2f(m_normal_debug_material.shader().uniform_location("chunk_translation"),
                0.0, 0.0);
    glUniform2f(m_normal_debug_material.shader().uniform_location("heightmap_base"),
                0.0, 0.0);
    m_normal_debug_material.attach_texture("heightmap", &m_heightmap);
    m_normal_debug_material.attach_texture("normalt", &m_normalt);
    glUniform1f(m_normal_debug_material.shader().uniform_location("normal_length"),
                2.);
    glUniform1f(m_normal_debug_material.shader().uniform_location("zoffset"),
                0.);
    RenderContext::configure_shader(m_normal_debug_material.shader());

    m_heightmap.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    m_normalt.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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

    m_unused_slots.reserve(m_texture_cache_size*m_texture_cache_size);
    m_heightmap_slots.resize(m_texture_cache_size*m_texture_cache_size);
    for (unsigned int i = 0;
         i < m_texture_cache_size*m_texture_cache_size;
         i++)
    {
        m_unused_slots.emplace_back(i);
    }
}

void FancyTerrainNode::collect_slices(
        std::vector<HeightmapSliceMeta> &requested_slices,
        const Vector3f &viewpoint)
{
#ifdef TIMELOG_SYNC
    const timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_lock;
#endif
    {
        const sim::MinMaxMapGenerator::MinMaxFieldLODs *minmaxfields = nullptr;
        auto lock = m_terrain_minmax.readonly_lods(minmaxfields);
#ifdef TIMELOG_SYNC
        t_lock = timelog_clock::now();
#endif

        collect_slices_recurse(requested_slices,
                               m_max_depth,
                               0,
                               0,
                               viewpoint,
                               *minmaxfields);
    }
#ifdef TIMELOG_SYNC
    logger.logf(io::LOG_DEBUG, "sync: collect_slices: time to lock: %.2f ms",
                ms_cast(t_lock - t0));
#endif
}

void FancyTerrainNode::collect_slices_recurse(
        std::vector<HeightmapSliceMeta> &requested_slices,
        const unsigned int lod,
        const unsigned int relative_x,
        const unsigned int relative_y,
        const Vector3f &viewpoint,
        const sim::MinMaxMapGenerator::MinMaxFieldLODs &minmaxfields)
{
    float min = sim::Terrain::min_height;
    float max = sim::Terrain::max_height;
    if (minmaxfields.size() > lod)
    {
        const unsigned int field_width = (m_terrain.size()-1) >> lod;
        std::tie(min, max) = minmaxfields[lod][relative_y*field_width+relative_x];
    }

    const unsigned int size = (1u << lod);

    const unsigned int absolute_x = relative_x * size;
    const unsigned int absolute_y = relative_y * size;

    AABB box{Vector3f(absolute_x, absolute_y, min),
             Vector3f(absolute_x+size, absolute_y+size, max)};

    const float next_range_radius = lod_range_base * (1u<<(lod-log2_of_pot(m_grid_size-1)));
    if (lod == log2_of_pot(m_grid_size-1) ||
            !isect_aabb_sphere(box, Sphere{viewpoint, next_range_radius}))
    {
        // next LOD not required, insert node
        requested_slices.push_back(
                    HeightmapSliceMeta{absolute_x, absolute_y, size}
                    );
        return;
    }

    // some children will need higher LOD

    for (unsigned int offsy = 0; offsy < 2; offsy++) {
        for (unsigned int offsx = 0; offsx < 2; offsx++) {
            collect_slices_recurse(
                        requested_slices,
                        lod-1,
                        relative_x*2+offsx,
                        relative_y*2+offsy,
                        viewpoint,
                        minmaxfields);
        }
    }
}

inline void render_slice(RenderContext &context,
                         VAO &vao,
                         Material &material,
                         IBOAllocation &ibo_allocation,
                         const float x, const float y, const float scale,
                         const unsigned int slot_index,
                         const unsigned int grid_size,
                         const unsigned int texture_cache_size)
{
    const float xtex = (float(slot_index % texture_cache_size) + 0.5/grid_size) / texture_cache_size;
    const float ytex = (float(slot_index / texture_cache_size) + 0.5/grid_size) / texture_cache_size;

    glUniform2f(material.shader().uniform_location("heightmap_base"),
                xtex, ytex);
    glUniform1f(material.shader().uniform_location("chunk_size"),
                scale);
    glUniform2f(material.shader().uniform_location("chunk_translation"),
                x, y);
    /* std::cout << "rendering" << std::endl;
    std::cout << "  xtex          = " << xtex << std::endl;
    std::cout << "  ytex          = " << ytex << std::endl;
    std::cout << "  translationx  = " << x << std::endl;
    std::cout << "  translationy  = " << y << std::endl;
    std::cout << "  scale         = " << scale << std::endl; */
    context.draw_elements(GL_TRIANGLES, vao, material, ibo_allocation);
}

void FancyTerrainNode::render_all(RenderContext &context, VAO &vao, Material &material)
{
    for (auto &slice: m_render_slices) {
        const float x = std::get<0>(slice).basex;
        const float y = std::get<0>(slice).basey;
        const float scale = std::get<0>(slice).lod;
        const unsigned int slot_index = std::get<1>(slice);

        render_slice(context, vao, material, m_ibo_allocation,
                     x, y, scale, slot_index,
                     m_grid_size, m_texture_cache_size);
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
    success = success && mat.shader().link();
    if (!success) {
        return false;
    }

    mat.attach_texture("heightmap", &m_heightmap);
    mat.attach_texture("normalt", &m_normalt);
    mat.shader().bind();
    glUniform1f(mat.shader().uniform_location("zoffset"), 1.0f);

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

void FancyTerrainNode::render(RenderContext &context)
{
#ifdef TIMELOG_RENDER
    const timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_solid, t_overlay;
#endif
    /* glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); */
    m_material.shader().bind();
    glUniform3fv(m_material.shader().uniform_location("lod_viewpoint"),
                 1, context.scene().viewpoint()/*Vector3f(0, 0, 0)*/.as_array);
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
                     1, context.scene().viewpoint()/*Vector3f(0, 0, 0)*/.as_array);
        for (auto &slice: m_render_slices)
        {
            const unsigned int x = std::get<0>(slice).basex;
            const unsigned int y = std::get<0>(slice).basey;
            const unsigned int scale = std::get<0>(slice).lod;
            const unsigned int slot_index = std::get<1>(slice);

            sim::TerrainRect slice_rect(x, y, x+scale, y+scale);
            if (slice_rect.overlaps(overlay.clip_rect)) {
                render_slice(context,
                             *m_vao, *overlay.material, m_ibo_allocation,
                             x, y, scale, slot_index,
                             m_grid_size, m_texture_cache_size);
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

void FancyTerrainNode::sync(Scene &scene)
{
    // FIXME: use SceneStorage here!

    const unsigned int texture_slots = m_texture_cache_size*m_texture_cache_size;

#ifdef TIMELOG_SYNC
    const timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_overlays, t_setup, t_allocate, t_upload;
#endif

    m_render_overlays.clear();
    m_render_overlays.reserve(m_overlays.size());
    for (auto &item: m_overlays) {
        m_render_overlays.emplace_back(
                    RenderOverlay{item.first, item.second.clip_rect}
                    );
    }

#ifdef TIMELOG_SYNC
    t_overlays = timelog_clock::now();
#endif

    m_heightmap.bind();
    m_tmp_slices.clear();
    collect_slices(m_tmp_slices, scene.viewpoint());
    m_render_slices.clear();

    unsigned int slot_index = 0;

#ifdef TIMELOG_SYNC
    t_setup = timelog_clock::now();
#endif

    for (auto &required_slice: m_tmp_slices)
    {
        if (slot_index == texture_slots) {
            logger.logf(io::LOG_ERROR,
                        "out of heightmap slots! (%u required in total)",
                        m_tmp_slices.size());
            break;
        }
        // FIXME: implement caching here!
        m_render_slices.emplace_back(required_slice, slot_index);

        slot_index++;
    }

#ifdef TIMELOG_SYNC
    t_allocate = timelog_clock::now();
#endif

    {
        const sim::Terrain::HeightField *lod0 = nullptr;
        const FancyTerrainInterface::HeightFieldLODifier::FieldLODs *lods = nullptr;
        auto hf_lods_lock = m_terrain_lods.readonly_lods(lods);
        auto hf_lock = m_terrain.readonly_field(lod0);

        for (auto &new_slice: m_render_slices)
        {
            const unsigned int lod_size = std::get<0>(new_slice).lod;
            const unsigned int lod_index = log2_of_pot(lod_size / (m_grid_size-1));

            // index zero is handled elsewhere
            if ((lods->size()+1) <= lod_index)
            {
                logger.logf(io::LOG_WARNING,
                            "cannot upload slice (no such LOD index: %u)",
                            lod_index);
                continue;
            }

            const unsigned int xlod = std::get<0>(new_slice).basex >> lod_index;
            const unsigned int ylod = std::get<0>(new_slice).basey >> lod_index;
            const unsigned int slot_index = std::get<1>(new_slice);

            const unsigned int xtex = (slot_index % m_texture_cache_size)*m_grid_size;
            const unsigned int ytex = (slot_index / m_texture_cache_size)*m_grid_size;

            const unsigned int src_size = ((m_terrain.size()-1) >> lod_index)+1;
            const sim::Terrain::HeightField &heightfield =
                    (lod_index == 0
                     ? *lod0
                     : (*lods)[lod_index-1]);

            /* std::cout << "uploading slice" << std::endl;
            std::cout << "  lod_size   = " << lod_size << std::endl;
            std::cout << "  lod_index  = " << lod_index << std::endl;
            std::cout << "  xlod       = " << xlod << std::endl;
            std::cout << "  ylod       = " << ylod << std::endl;
            std::cout << "  slot_index = " << slot_index << std::endl;
            std::cout << "  xtex       = " << xtex << std::endl;
            std::cout << "  ytex       = " << ytex << std::endl;
            std::cout << "  src_size   = " << src_size << std::endl;
            std::cout << "  dest_size  = " << m_grid_size << std::endl;
            std::cout << "  top left   = " << heightfield[0] << std::endl;
            std::cout << "  vec size   = " << heightfield.size() << std::endl; */

            glPixelStorei(GL_UNPACK_ROW_LENGTH, src_size);
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            xtex,
                            ytex,
                            m_grid_size,
                            m_grid_size,
                            GL_RED,
                            GL_FLOAT,
                            &heightfield.data()[ylod*src_size+xlod]);
        }
    }

    m_normalt.bind();
    {
        const sim::NTMapGenerator::NTField *nt_lod0 = nullptr;
        const FancyTerrainInterface::NTMapLODifier::FieldLODs *nt_lods = nullptr;
        auto nt_lods_lock = m_terrain_nt_lods.readonly_lods(nt_lods);
        auto nt_lock = m_terrain_nt.readonly_field(nt_lod0);

        for (auto &new_slice: m_render_slices)
        {
            const unsigned int lod_size = std::get<0>(new_slice).lod;
            const unsigned int lod_index = log2_of_pot(lod_size / (m_grid_size-1));

            // index zero is handled elsewhere
            if ((nt_lods->size()+1) <= lod_index
                    || (lod_index == 0 && nt_lod0->size() == 0))
            {
                logger.logf(io::LOG_WARNING,
                            "cannot upload NT slice (no such LOD index: %u)",
                            lod_index);
                continue;
            }

            const unsigned int xlod = std::get<0>(new_slice).basex >> lod_index;
            const unsigned int ylod = std::get<0>(new_slice).basey >> lod_index;
            const unsigned int slot_index = std::get<1>(new_slice);

            const unsigned int xtex = (slot_index % m_texture_cache_size)*m_grid_size;
            const unsigned int ytex = (slot_index / m_texture_cache_size)*m_grid_size;

            const unsigned int src_size = ((m_terrain.size()-1) >> lod_index)+1;
            const sim::NTMapGenerator::NTField &ntfield =
                    (lod_index == 0
                     ? *nt_lod0
                     : (*nt_lods)[lod_index-1]);

            /* std::cout << "uploading slice" << std::endl;
            std::cout << "  lod_size   = " << lod_size << std::endl;
            std::cout << "  lod_index  = " << lod_index << std::endl;
            std::cout << "  xlod       = " << xlod << std::endl;
            std::cout << "  ylod       = " << ylod << std::endl;
            std::cout << "  slot_index = " << slot_index << std::endl;
            std::cout << "  xtex       = " << xtex << std::endl;
            std::cout << "  ytex       = " << ytex << std::endl;
            std::cout << "  src_size   = " << src_size << std::endl;
            std::cout << "  dest_size  = " << m_grid_size << std::endl;
            std::cout << "  top left   = " << ntfield[0] << std::endl;
            std::cout << "  vec size   = " << ntfield.size() << std::endl; */

            glPixelStorei(GL_UNPACK_ROW_LENGTH, src_size);
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            xtex,
                            ytex,
                            m_grid_size,
                            m_grid_size,
                            GL_RGBA,
                            GL_FLOAT,
                            &ntfield.data()[ylod*src_size+xlod]);
        }
    }

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    m_vao->sync();
#ifdef TIMELOG_SYNC
    t_upload = timelog_clock::now();
    logger.logf(io::LOG_DEBUG, "sync: t_overlays = %.2f ms", ms_cast(t_overlays - t0).count());
    logger.logf(io::LOG_DEBUG, "sync: t_setup    = %.2f ms", ms_cast(t_setup - t_overlays).count());
    logger.logf(io::LOG_DEBUG, "sync: t_allocate = %.2f ms", ms_cast(t_allocate - t_setup).count());
    logger.logf(io::LOG_DEBUG, "sync: t_upload   = %.2f ms", ms_cast(t_upload - t_allocate).count());
#endif
}


}
