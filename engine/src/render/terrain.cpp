#include "engine/render/terrain.hpp"

#include <unistd.h>

namespace engine {

const unsigned int Terrain::CHUNK_SIZE = 64;
const unsigned int Terrain::CHUNK_VERTICIES = (CHUNK_SIZE+1)*(CHUNK_SIZE+1);
const VBOFormat Terrain::VBO_FORMAT({
                                        VBOAttribute(3),  // position
                                        VBOAttribute(3),  // normal
                                        VBOAttribute(3),  // tangent
                                        VBOAttribute(2),  // tex coord
                                    });

static bool dump = false;

Terrain::Terrain(sim::Terrain &src):
    scenegraph::Node(),
    m_xchunks((src.m_width-1) / CHUNK_SIZE),
    m_ychunks((src.m_height-1) / CHUNK_SIZE),
    m_source(src),
    m_vbo(VBO_FORMAT),
    m_ibo(),
    m_chunk_indicies(m_ibo.allocate((CHUNK_SIZE+1)*(CHUNK_SIZE+1)*6)),
    m_chunks()
{
    if (m_xchunks*CHUNK_SIZE+1 != src.m_width ||
            m_ychunks*CHUNK_SIZE+1 != src.m_height)
    {
        throw std::invalid_argument(
                    "Terrain size minus one (" + std::to_string(src.m_width-1) + "×" + std::to_string(src.m_height-1) +
                    ") not divisible by chunk size " + std::to_string(CHUNK_SIZE));
    }

    uint16_t *dest = m_chunk_indicies.get();
    for (unsigned int x = 0; x < CHUNK_SIZE; x++)
    {
        for (unsigned int y = 0; y < CHUNK_SIZE; y++)
        {
            const unsigned int curr_base = x*(CHUNK_SIZE+1)+y;
            // make a triangle
            *dest++ = curr_base + (CHUNK_SIZE+1);
            *dest++ = curr_base + (CHUNK_SIZE+1) + 1;
            *dest++ = curr_base;

            *dest++ = curr_base;
            *dest++ = curr_base + (CHUNK_SIZE+1) + 1;
            *dest++ = curr_base + 1;
        }
    }

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_vbo, 0);
    decl.declare_attribute("normal", m_vbo, 1);
    decl.declare_attribute("tangent", m_vbo, 2);
    decl.declare_attribute("texcoord0", m_vbo, 3);
    decl.set_ibo(&m_ibo);

    bool success = m_material.shader().attach(
                GL_VERTEX_SHADER,
                "#version 330\n"
                "layout(std140) uniform MatrixBlock {"
                "   layout(row_major) mat4 proj;"
                "   layout(row_major) mat4 view;"
                "   layout(row_major) mat4 model;"
                "   layout(row_major) mat3 normal;"
                "} mats;"
                "in vec3 position;"
                "in vec2 texcoord0;"
                "in vec3 normal;"
                "out vec2 tc0;"
                "out vec3 norm;"
                "void main() {"
                "   tc0 = texcoord0;"
                "   norm = normal;"
                "   gl_Position = mats.proj * mats.view * mats.model * vec4(position, 1.f);"
                "}");
    success = success && m_material.shader().attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "out vec4 color;"
                "in vec2 tc0;"
                "in vec3 norm;"
                "uniform sampler2D grass;"
                "void main() {"
                "   color = texture2D(grass, tc0);"
                "}");
    success = success && m_material.shader().link();

    m_material.shader().bind();

    if (!success) {
        throw std::runtime_error("failed to compile or link shader");
    }

    for (unsigned int xc = 0; xc < m_xchunks; xc++)
    {
        for (unsigned int yc = 0; yc < m_ychunks; yc++)
        {
            m_chunks.push_back(m_vbo.allocate(CHUNK_VERTICIES));
        }
    }

    m_vao = decl.make_vao(m_material.shader(), true);
}

void Terrain::set_grass_texture(Texture2D *tex)
{
    m_material.attach_texture("grass", tex);
}

void Terrain::sync_from_sim()
{
    unsigned int i = 0;
    for (unsigned int xchunk = 0; xchunk < m_xchunks; xchunk++)
    {
        for (unsigned int ychunk = 0; ychunk < m_ychunks; ychunk++)
        {
            sync_chunk_from_sim(xchunk, ychunk, m_chunks[i]);
            i++;
        }
    }
}

void Terrain::sync_chunk_from_sim(const unsigned int xchunk,
                                  const unsigned int ychunk,
                                  VBOAllocation &vbo_alloc)
{
    auto pos_slice = VBOSlice<Vector3f>(vbo_alloc, 0);
    auto normal_slice = VBOSlice<Vector3f>(vbo_alloc, 1);
    auto tangent_slice = VBOSlice<Vector3f>(vbo_alloc, 2);
    auto texcoord_slice = VBOSlice<Vector2f>(vbo_alloc, 3);

    const unsigned int xbase = xchunk*CHUNK_SIZE;
    const unsigned int ybase = ychunk*CHUNK_SIZE;
    unsigned int slice_index = 0;

    const float texcoord_factor = 1./5.;

    for (unsigned int x = 0; x <= CHUNK_SIZE; x++)
    {
        const unsigned int absx = xbase+x;
        for (unsigned int y = 0; y <= CHUNK_SIZE; y++)
        {
            const unsigned int absy = ybase+y;
            const sim::Terrain::height_t h = m_source.get(absx, absy);
            if (h != 20) {
                dump = true;
            }
            pos_slice[slice_index] = Vector3f(absx, absy, h);
            normal_slice[slice_index] = Vector3f(0, 0, 1.);
            tangent_slice[slice_index] = Vector3f(1, 0, 0);
            texcoord_slice[slice_index] = Vector2f(absx*texcoord_factor,
                                                   absy*texcoord_factor);
            slice_index += 1;
        }
    }

    vbo_alloc.mark_dirty();
}

void Terrain::render(RenderContext &context)
{
    for (unsigned int i = 0; i < m_xchunks*m_ychunks; i++)
    {
        context.draw_elements_base_vertex(
                    GL_TRIANGLES, *m_vao, m_material, m_chunk_indicies,
                    m_chunks[i].base());
    }

}

void Terrain::sync()
{
    sync_from_sim();
    m_vao->sync();
}

}
