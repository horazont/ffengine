#include "engine/render/fancyterrain.hpp"

#include "engine/common/utils.hpp"
#include "engine/math/intersect.hpp"
#include "engine/io/log.hpp"

namespace engine {

io::Logger &logger = io::logging().get_logger("engine.render.fancyterrain");

bool HeightmapSliceMeta::operator<(const HeightmapSliceMeta &other) const
{
    return lod < other.lod;
}


FancyTerrainNode::FancyTerrainNode(sim::Terrain &terrain,
                                   const unsigned int grid_size,
                                   const unsigned int texture_cache_size):
    m_grid_size(grid_size),
    m_texture_cache_size(texture_cache_size),
    m_min_lod(grid_size-1),
    m_max_depth(log2_of_pot((terrain.size()-1) / m_min_lod)),
    m_terrain(terrain),
    m_terrain_lods(terrain),
    m_terrain_minmax(terrain, m_min_lod+1),
    m_terrain_lods_conn(terrain.terrain_changed().connect(
                            std::bind(&HeightFieldLODifier::notify,
                                      &m_terrain_lods))),
    m_terrain_minmax_conn(terrain.terrain_changed().connect(
                              std::bind(&sim::MinMaxMapGenerator::notify,
                                        &m_terrain_minmax))),
    m_heightmap(GL_R32F,
                texture_cache_size*grid_size,
                texture_cache_size*grid_size,
                GL_RED,
                GL_FLOAT),
    m_vbo(VBOFormat({
                        VBOAttribute(2)
                    })),
    m_ibo(),
    m_vbo_allocation(m_vbo.allocate(grid_size*grid_size)),
    m_ibo_allocation(m_ibo.allocate((grid_size-1)*(grid_size-1)*6))
{
    m_terrain_lods.notify();
    m_terrain_minmax.notify();

    if (!is_power_of_two(grid_size-1)) {
        throw std::runtime_error("grid_size must be power-of-two plus one");
    }
    if (((terrain.size()-1) / (m_grid_size-1))*(m_grid_size-1) != terrain.size()-1)
    {
        throw std::runtime_error("grid_size-1 must divide terrain size-1 evenly");
    }

    uint16_t *dest = m_ibo_allocation.get();
    for (unsigned int y = 0; y < grid_size-1; y++) {
        for (unsigned int x = 0; x < grid_size-1; x++) {
            const unsigned int curr_base = y*grid_size + x;
            *dest++ = curr_base + grid_size;
            *dest++ = curr_base + grid_size + 1;
            *dest++ = curr_base;

            *dest++ = curr_base;
            *dest++ = curr_base + grid_size + 1;
            *dest++ = curr_base + 1;
        }
    }

    m_ibo_allocation.mark_dirty();

    bool success = true;

    success = success && m_material.shader().attach(
                GL_VERTEX_SHADER,
                "#version 330\n"
                "layout(std140) uniform MatrixBlock {"
                "   layout(row_major) mat4 proj;"
                "   layout(row_major) mat4 view;"
                "   layout(row_major) mat4 model;"
                "   layout(row_major) mat3 normal;"
                "} mats;"
                "uniform float chunk_size;"
                "uniform vec2 chunk_translation;"
                "uniform vec2 heightmap_base;"
                "uniform sampler2D heightmap;"
                "uniform vec3 lod_viewpoint;"
                "const float heightmap_factor = " + std::to_string(float(m_grid_size-1) / (m_grid_size*m_texture_cache_size)) +
                ";"
                "const float grid_size = " + std::to_string(m_grid_size-1) +
                ";"
                "const float scale_to_radius = " + std::to_string(lod_range_base / (m_grid_size-1)) +
                ";"
                "in vec2 position;"
                "out vec2 tc0;"
                "vec2 morph_vertex(vec2 grid_pos, vec2 vertex, float morph_k)"
                "{"
                "   vec2 frac_part = fract(grid_pos.xy * grid_size * 0.5) * 2.0 / grid_size;"
                "   if (grid_pos.x == 1.0) { frac_part.x = 0; }"
                "   if (grid_pos.y == 1.0) { frac_part.y = 0; }"
                "   return vertex.xy - frac_part * chunk_size * morph_k;"
                "}"
                "float morph_k(vec3 viewpoint, vec2 world_pos)"
                "{"
                "   float dist = length(viewpoint - vec3(world_pos, 0)) - chunk_size*scale_to_radius/2.f;"
                "   float normdist = dist/(scale_to_radius*chunk_size);"
                "   return clamp((normdist - 0.6) * 2.5, 0, 1);"
                "}"
                "void main() {"
                "   vec2 model_vertex = position * chunk_size + chunk_translation;"
                "   tc0 = model_vertex.xy / 5.0;"
                "   vec2 morphed = morph_vertex(position, model_vertex, morph_k(lod_viewpoint, model_vertex));"
                "   vec2 morphed_object = (morphed - chunk_translation) / chunk_size;"
                "   float height = textureLod(heightmap, heightmap_base + morphed_object.xy * heightmap_factor, 0).r;"
                "   gl_Position = mats.proj * mats.view * mats.model * vec4("
                "       morphed, height, 1.f);"
                "}");
    success = success && m_material.shader().attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "out vec4 color;"
                "in vec2 tc0;"
                "uniform sampler2D heightmap;"
                "void main() {"
                "   color = vec4(0.5, 0.5, 0.5, 0.4);"
                "}");
    success = success && m_material.shader().link();

    if (!success) {
        throw std::runtime_error("failed to compile or link shader");
    }

    m_material.shader().bind();
    glUniform2f(m_material.shader().uniform_location("chunk_translation"),
                0.0, 0.0);
    glUniform2f(m_material.shader().uniform_location("heightmap_base"),
                0.0, 0.0);
    m_material.attach_texture("heightmap", &m_heightmap);

    m_heightmap.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);

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

    m_unused_slots.reserve(m_texture_cache_size*m_texture_cache_size);
    m_heightmap_slots.resize(m_texture_cache_size*m_texture_cache_size);
    for (unsigned int i = 0;
         i < m_texture_cache_size*m_texture_cache_size;
         i++)
    {
        m_unused_slots.emplace_back(i);
    }
}

FancyTerrainNode::~FancyTerrainNode()
{
    m_terrain_lods_conn.disconnect();
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
        std::cout << field_width << std::endl;
        std::tie(min, max) = minmaxfields[lod][relative_y*field_width+relative_x];
        std::cout << min << " " << max << std::endl;
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
                    HeightmapSliceMeta{absolute_x, absolute_y, m_min_lod * (1<<lod)}
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

void FancyTerrainNode::render_all(RenderContext &context)
{
    for (auto &slice: m_render_slices) {
        const float x = std::get<0>(slice).basex;
        const float y = std::get<0>(slice).basey;
        const float scale = std::get<0>(slice).lod;
        const unsigned int slot_index = std::get<1>(slice);
        const float xtex = (float(slot_index % m_texture_cache_size) + 0.5/m_grid_size) / m_texture_cache_size;
        const float ytex = (float(slot_index / m_texture_cache_size) + 0.5/m_grid_size) / m_texture_cache_size;

        glUniform2f(m_material.shader().uniform_location("heightmap_base"),
                    xtex, ytex);
        glUniform1f(m_material.shader().uniform_location("chunk_size"),
                    scale);
        glUniform2f(m_material.shader().uniform_location("chunk_translation"),
                    x, y);
        /* std::cout << "rendering" << std::endl;
        std::cout << "  xtex          = " << xtex << std::endl;
        std::cout << "  ytex          = " << ytex << std::endl;
        std::cout << "  translationx  = " << x << std::endl;
        std::cout << "  translationy  = " << y << std::endl;
        std::cout << "  scale         = " << scale << std::endl; */
        context.draw_elements(GL_TRIANGLES, *m_vao, m_material, m_ibo_allocation);
    }
}

void FancyTerrainNode::render(RenderContext &context)
{
    m_material.shader().bind();
    glUniform3fv(m_material.shader().uniform_location("lod_viewpoint"),
                 1, m_render_viewpoint.as_array);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_BLEND);
    render_all(context);
    glEnable(GL_BLEND);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    render_all(context);
}

void FancyTerrainNode::sync()
{
    const unsigned int texture_slots = m_texture_cache_size*m_texture_cache_size;

    m_heightmap.bind();
    m_tmp_slices.clear();
    m_render_viewpoint = Vector3f(m_terrain.size()/2.f, m_terrain.size()/2.f, 0);
    collect_slices(m_tmp_slices, m_render_viewpoint);
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
        const HeightFieldLODifier::FieldLODs *lods = nullptr;
        auto hf_lods_lock = m_terrain_lods.readonly_lods(lods);
        auto hf_lock = m_terrain.readonly_field(lod0);

        sim::Terrain::HeightField tmp_heightfield(m_grid_size*m_grid_size);
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

            /* std::cout << "uploading slice" << std::endl;
            std::cout << "  lod_size   = " << lod_size << std::endl;
            std::cout << "  lod_index  = " << lod_index << std::endl;
            std::cout << "  xlod       = " << xlod << std::endl;
            std::cout << "  ylod       = " << ylod << std::endl;
            std::cout << "  slot_index = " << slot_index << std::endl;
            std::cout << "  xtex       = " << xtex << std::endl;
            std::cout << "  ytex       = " << ytex << std::endl;
            std::cout << "  src_size   = " << (((m_terrain.size()-1) >> lod_index)+1) << std::endl;
            std::cout << "  dest_size  = " << m_grid_size << std::endl;
            std::cout << "  top left   = " << (*lods)[lod_index][0] << std::endl; */

            const sim::Terrain::HeightField &field = (
                        lod_index == 0
                        ? *lod0
                        : (*lods)[lod_index-1]);

            sim::copy_heightfield_rect(
                        field,
                        xlod,
                        ylod,
                        ((m_terrain.size()-1) >> lod_index)+1,
                        tmp_heightfield,
                        m_grid_size,
                        m_grid_size);

            glTexSubImage2D(
                        GL_TEXTURE_2D,
                        0,
                        xtex,
                        ytex,
                        m_grid_size,
                        m_grid_size,
                        GL_RED,
                        GL_FLOAT,
                        tmp_heightfield.data()
                        );
        }
    }

    m_vao->sync();

}

}
