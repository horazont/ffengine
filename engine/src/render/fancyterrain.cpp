#include "engine/render/fancyterrain.hpp"

#include "engine/common/utils.hpp"
#include "engine/math/intersect.hpp"
#include "engine/io/log.hpp"

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
    m_max_depth(log2_of_pot((terrain_interface.size()-1) / m_min_lod)),
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
    const sim::MinMaxMapGenerator::MinMaxFieldLODs *minmaxfields = nullptr;
    auto lock = m_terrain_minmax.readonly_lods(minmaxfields);

    collect_slices_recurse(requested_slices,
                           m_max_depth,
                           0,
                           0,
                           viewpoint,
                           *minmaxfields);
}

void FancyTerrainNode::collect_slices_recurse(
        std::vector<HeightmapSliceMeta> &requested_slices,
        const unsigned int lod,
        const unsigned int relative_x,
        const unsigned int relative_y,
        const Vector3f &viewpoint,
        const sim::MinMaxMapGenerator::MinMaxFieldLODs &minmaxfields)
{
    float min = 0;
    float max = 100.;
    if (minmaxfields.size() > lod)
    {
        const unsigned int field_width = ((m_terrain.size()-1) / m_min_lod) >> lod;
        std::tie(min, max) = minmaxfields[lod][relative_y*field_width+relative_x];
    }

    const unsigned int size = (m_min_lod << lod);

    const unsigned int absolute_x = relative_x * size;
    const unsigned int absolute_y = relative_y * size;

    AABB box{Vector3f(absolute_x, absolute_y, min),
             Vector3f(absolute_x+size, absolute_y+size, max)};

    const float next_range_radius = lod_range_base * (1<<lod);
    if (lod == 0 ||
            !isect_aabb_sphere(box, Sphere{viewpoint, next_range_radius}))
    {
        // next LOD not required, insert node
        requested_slices.push_back(
                    HeightmapSliceMeta{absolute_x, absolute_y, m_min_lod << lod}
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

void FancyTerrainNode::attach_grass_texture(Texture2D *tex)
{
    m_material.attach_texture("grass", tex);
    m_normal_debug_material.attach_texture("grass", tex);
}

void FancyTerrainNode::render(RenderContext &context)
{
    /* glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); */
    m_material.shader().bind();
    glUniform3fv(m_material.shader().uniform_location("lod_viewpoint"),
                 1, context.scene().viewpoint().as_array);
    render_all(context, *m_vao, m_material);
    /* glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); */

    /* m_normal_debug_material.shader().bind();
    glUniform3fv(m_normal_debug_material.shader().uniform_location("lod_viewpoint"),
                 1, context.scene().viewpoint().as_array);
    render_all(context, *m_nd_vao, m_normal_debug_material); */
}

void FancyTerrainNode::sync(Scene &scene)
{
    // FIXME: use SceneStorage here!

    const unsigned int texture_slots = m_texture_cache_size*m_texture_cache_size;

    m_heightmap.bind();
    m_tmp_slices.clear();
    collect_slices(m_tmp_slices, scene.viewpoint());
    m_render_slices.clear();

    unsigned int slot_index = 0;

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
}


}
