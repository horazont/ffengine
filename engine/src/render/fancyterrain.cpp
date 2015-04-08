#include "engine/render/fancyterrain.hpp"

#include "engine/common/utils.hpp"
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
    m_max_lod(terrain.size()-1),
    m_min_lod(grid_size-1),
    m_terrain(terrain),
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
                "const float heightmap_factor = " + std::to_string(float(m_grid_size-1) / (m_grid_size*m_texture_cache_size)) + ";"
                "in vec2 position;"
                "out vec2 tc0;"
                "void main() {"
                "   tc0 = position.xy / 5.0;"
                "   float height = textureLod(heightmap, heightmap_base + position.xy * heightmap_factor, 0).r;"
                "   gl_Position = mats.proj * mats.view * mats.model * vec4("
                "       position * chunk_size + chunk_translation, height, 1.f);"
                "}");
    success = success && m_material.shader().attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "out vec4 color;"
                "in vec2 tc0;"
                "uniform sampler2D heightmap;"
                "void main() {"
                "   color = vec4(0.5, 0.5, 0.5, 1.0);"
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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

void FancyTerrainNode::collect_slices(
        std::vector<HeightmapSliceMeta> &requested_slices,
        const Vector3f &viewpoint)
{
    collect_slices_recurse(requested_slices,
                           Vector<unsigned int, 2>(m_max_lod / 2, m_max_lod / 2),
                           m_max_lod,
                           viewpoint);
}

void FancyTerrainNode::collect_slices_recurse(
        std::vector<HeightmapSliceMeta> &requested_slices,
        const Vector<unsigned int, 2> &center,
        unsigned int lod,
        const Vector3f &viewpoint)
{
    if (lod <= m_min_lod) {
        requested_slices.push_back(
                    HeightmapSliceMeta{center[eX]-lod/2, center[eY]-lod/2, lod}
                    );
        return;
    }
    typedef Vector<unsigned int, 2> center_t;

    collect_slices_recurse(requested_slices,
                           center_t(
                               center[eX]-lod/4,
                               center[eY]-lod/4),
                           lod/2,
                           viewpoint);
    collect_slices_recurse(requested_slices,
                           center_t(
                               center[eX]-lod/4,
                               center[eY]+lod/4),
                           lod/2,
                           viewpoint);
    collect_slices_recurse(requested_slices,
                           center_t(
                               center[eX]+lod/4,
                               center[eY]-lod/4),
                           lod/2,
                           viewpoint);
    collect_slices_recurse(requested_slices,
                           center_t(
                               center[eX]+lod/4,
                               center[eY]+lod/4),
                           lod/2,
                           viewpoint);
}

void FancyTerrainNode::render(RenderContext &context)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (auto &slice: m_render_slices) {
        const float x = std::get<0>(slice).basex;
        const float y = std::get<0>(slice).basey;
        const float scale = std::get<0>(slice).lod;
        const unsigned int slot_index = std::get<1>(slice);
        const float xtex = (float(slot_index % m_texture_cache_size) + 0.5/m_grid_size) / m_texture_cache_size;
        const float ytex = (float(slot_index / m_texture_cache_size) + 0.5/m_grid_size) / m_texture_cache_size;

        m_material.shader().bind();
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
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void FancyTerrainNode::sync()
{
    const unsigned int texture_slots = m_texture_cache_size*m_texture_cache_size;

    m_heightmap.bind();
    m_tmp_slices.clear();
    collect_slices(m_tmp_slices, Vector3f(0, 0, 0));
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
        const sim::Terrain::HeightFieldLODs *lods = nullptr;
        auto lock = m_terrain.readonly_lods(lods);

        sim::Terrain::HeightField tmp_heightfield(m_grid_size*m_grid_size);
        for (auto &new_slice: m_render_slices)
        {
            const unsigned int lod_size = std::get<0>(new_slice).lod;
            const unsigned int lod_index = log2_of_pot(lod_size / (m_grid_size-1));

            if (lods->size() <= lod_index)
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

            sim::copy_heightfield_rect(
                        (*lods)[lod_index],
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
