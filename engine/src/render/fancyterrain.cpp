#include "engine/render/fancyterrain.hpp"

namespace engine {

bool HeightmapSliceMeta::operator<(const HeightmapSliceMeta &other) const
{
    return lod < other.lod;
}


FancyTerrainNode::FancyTerrainNode(sim::Terrain &terrain,
                                   const unsigned int grid_size,
                                   const unsigned int texture_cache_size):
    m_grid_size(grid_size),
    m_texture_cache_size(texture_cache_size),
    m_min_lod(terrain.size() / grid_size),
    m_max_lod(terrain.size()),
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
    m_ibo_allocation(m_ibo.allocate(grid_size*grid_size*6))
{
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

    m_heightmap.bind();
    std::basic_string<float> buf(64*64, 20.);
    raise_last_gl_error();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RED, GL_FLOAT, buf.data());
    raise_last_gl_error();

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
                "in vec2 position;"
                "out vec2 tc0;"
                "void main() {"
                "   tc0 = position.xy / 5.0;"
                "   float height = textureLod(heightmap, heightmap_base + position.xy, 0).r;"
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
    glUniform1f(m_material.shader().uniform_location("chunk_size"),
                64.0);
    glUniform2f(m_material.shader().uniform_location("chunk_translation"),
                0.0, 0.0);
    glUniform2f(m_material.shader().uniform_location("heightmap_base"),
                0.0, 0.0);
    m_material.attach_texture("heightmap", &m_heightmap);

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.set_ibo(&m_ibo);

    m_vao = decl.make_vao(m_material.shader(), true);

    {
        auto slice = VBOSlice<Vector2f>(m_vbo_allocation, 0);
        unsigned int index = 0;
        for (unsigned int y = 0; y < grid_size; y++) {
            for (unsigned int x = 0; x < grid_size; x++) {
                slice[index++] = Vector2f(x, y) / grid_size;
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

}

void FancyTerrainNode::render(RenderContext &context)
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    context.draw_elements(GL_TRIANGLES, *m_vao, m_material, m_ibo_allocation);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void FancyTerrainNode::sync()
{
    m_vao->sync();
    m_heightmap.bind();
    m_vao->sync();
    const sim::Terrain::HeightField *heightmap = nullptr;
    auto lock = m_terrain.readonly_heightmap(heightmap);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RED, GL_FLOAT,
                    heightmap->data());
}

}
