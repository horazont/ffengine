#include "engine/render/quadterrain.hpp"

#include "engine/io/log.hpp"

namespace engine {

static io::Logger &log = io::logging().get_logger("engine.render.quadterrain");

QuadTerrainChunk::QuadTerrainChunk(VBO &pos_vbo, VBO &normal_vbo, IBO &ibo):
    m_pos_vbo(pos_vbo),
    m_normal_vbo(normal_vbo),
    m_ibo(ibo),
    m_mesh(),
    m_vertex_cache()
{
    m_mesh.add_vertex_property<surface_mesh::Point>("v:normal");
    m_mesh.add_vertex_property<surface_mesh::Point>("f:normal");
}

surface_mesh::Surface_mesh::Vertex QuadTerrainChunk::add_vertex_cached(
        const sim::TerrainVector &vec)
{
    auto iter = m_vertex_cache.find(vec);
    if (iter == m_vertex_cache.end()) {
        surface_mesh::Surface_mesh::Vertex v = m_mesh.add_vertex(
                    surface_mesh::Point(vec[eX], vec[eY], vec[eZ])
                    );
        m_vertex_cache[vec] = v;
        return v;
    }
    return iter->second;
}

void QuadTerrainChunk::build_from_quadtree(
        sim::QuadNode *root,
        sim::QuadNode *subtree_root,
        unsigned int lod,
        const std::array<unsigned int, 4> &neighbour_lod)
{
    typedef surface_mesh::Surface_mesh::Vertex Vertex;

    switch (subtree_root->type())
    {
    case sim::QuadNode::Type::LEAF:
    {
        const float size = subtree_root->size();
        const sim::terrain_height_t height = subtree_root->height();
        const sim::TerrainVector base(subtree_root->x0(),
                                      subtree_root->y0(),
                                      subtree_root->height());

        const Vertex nw = add_vertex_cached(base);
        const Vertex ne = add_vertex_cached(base + sim::TerrainVector(size-1, 0, 0));
        const Vertex sw = add_vertex_cached(base + sim::TerrainVector(0, size-1, 0));
        const Vertex se = add_vertex_cached(base + sim::TerrainVector(size-1, size-1, 0));

        if (size > 1) {
            // inner quad
            make_quad(nw, ne, sw, se);
        }

        // eastern seam

        root->sample_line(m_tmp_terrain_vectors,
                          subtree_root->x0()+size,
                          subtree_root->y0(),
                          sim::QuadNode::SAMPLE_SOUTH,
                          size);
        if (!m_tmp_terrain_vectors.empty())
        {
            Vertex prev = add_vertex_cached(m_tmp_terrain_vectors[0]);
            for (auto iter = m_tmp_terrain_vectors.begin()+1;
                 iter != m_tmp_terrain_vectors.end();
                 ++iter)
            {
                Vertex curr = add_vertex_cached(*iter);

                make_quad(ne, prev, se, curr);

                prev = curr;
            }
            m_tmp_terrain_vectors.clear();
        }

        break;
    }
    case sim::QuadNode::Type::NORMAL:
    {
        for (unsigned int i = 0; i < 4; i++) {
            build_from_quadtree(
                        root,
                        subtree_root->child(i),
                        lod,
                        neighbour_lod);
        }
        break;
    }
    }
}

void QuadTerrainChunk::make_quad(surface_mesh::Surface_mesh::Vertex v1,
                                 surface_mesh::Surface_mesh::Vertex v2,
                                 surface_mesh::Surface_mesh::Vertex v3,
                                 surface_mesh::Surface_mesh::Vertex v4)
{
    m_mesh.add_triangle(v1, v3, v2);
    m_mesh.add_triangle(v2, v3, v4);
}

void QuadTerrainChunk::update(
        sim::QuadNode *root,
        sim::QuadNode *subtree_root,
        unsigned int lod,
        const std::array<unsigned int, 4> &neighbour_lod)
{
    m_mesh.clear();
    m_vertex_cache.clear();
    m_mesh.garbage_collection();
    build_from_quadtree(root, subtree_root, lod, neighbour_lod);
}

void QuadTerrainChunk::update(const sim::terrain_coord_t x0,
                              const sim::terrain_coord_t y0,
                              const sim::terrain_coord_t size,
                              const sim::terrain_height_t height)
{
    typedef surface_mesh::Surface_mesh::Vertex Vertex;

    std::cout << x0 << " " << y0 << " " << size << " " << height << std::endl;
    m_mesh.clear();
    m_vertex_cache.clear();
    m_mesh.garbage_collection();

    std::array<Vertex, 4> vertices({
                                       m_mesh.add_vertex(surface_mesh::Point(x0, y0, height)),
                                       m_mesh.add_vertex(surface_mesh::Point(x0+size, y0, height)),
                                       m_mesh.add_vertex(surface_mesh::Point(x0, y0+size, height)),
                                       m_mesh.add_vertex(surface_mesh::Point(x0+size, y0+size, height))
                                   });
    make_quad(vertices[0], vertices[1], vertices[2], vertices[3]);
}

void QuadTerrainChunk::release_buffers()
{
    m_pos_alloc = nullptr;
    m_normal_alloc = nullptr;
    m_ialloc = nullptr;
}

void QuadTerrainChunk::mesh_to_buffers()
{
    m_vertex_cache.clear();
    m_mesh.garbage_collection();

    m_mesh.update_face_normals();
    m_mesh.update_vertex_normals();

    m_pos_alloc = nullptr;
    m_normal_alloc = nullptr;
    m_ialloc = nullptr;

    m_pos_alloc = std::move(m_pos_vbo.allocate(m_mesh.n_vertices()));
    m_normal_alloc = std::move(m_normal_vbo.allocate(m_mesh.n_vertices()));
    m_ialloc = std::move(m_ibo.allocate(m_mesh.n_faces()*3));

    auto positions = m_mesh.get_vertex_property<surface_mesh::Point>("v:point");
    auto normals = m_mesh.get_vertex_property<surface_mesh::Point>("v:normal");
    auto pos_slice = VBOSlice<Vector3f>(m_pos_alloc, 0);
    auto normal_slice = VBOSlice<Vector3f>(m_normal_alloc, 0);
    for (auto vertex: m_mesh.vertices())
    {
        {
            const surface_mesh::Point &point = positions[vertex];
            pos_slice[vertex.idx()] = Vector3f(point[0], point[1], point[2]);
        }
        {
            const surface_mesh::Point &point = normals[vertex];
            normal_slice[vertex.idx()] = Vector3f(point[0], point[1], point[2]);
        }
    }

    uint16_t *dest = m_ialloc.get();
    for (auto face: m_mesh.faces())
    {
        for (auto vertex: m_mesh.vertices(face))
        {
            *dest++ = vertex.idx();
        }
    }
}


QuadTerrainNode::QuadTerrainNode(sim::QuadNode *root, unsigned int chunk_lod):
    m_xchunks(root->size() / chunk_lod),
    m_ychunks(root->size() / chunk_lod),
    m_chunk_lod(chunk_lod),
    m_root(root),
    m_pos_vbo(VBOFormat({
                        VBOAttribute(3)
                        })),
    m_normal_vbo(VBOFormat({
                        VBOAttribute(3)
                    })),
    m_ibo(),
    m_chunks()
{

    ArrayDeclaration decl;
    decl.declare_attribute("position", m_pos_vbo, 0);
    decl.declare_attribute("normal", m_normal_vbo, 1);
    decl.set_ibo(&m_ibo);

    std::string vertex_shader(
                "#version 330\n"
                "layout(std140) uniform MatrixBlock {"
                "   layout(row_major) mat4 proj;"
                "   layout(row_major) mat4 view;"
                "   layout(row_major) mat4 model;"
                "   layout(row_major) mat3 normal;"
                "} mats;"
                "in vec3 position;"
                "in vec3 normal;"
                "out vec2 tc0;"
                "out vec3 norm;"
                "void main() {"
                "   tc0 = position.xy / 5.0;"
                "   norm = normal;"
                "   gl_Position = mats.proj * mats.view * mats.model * vec4(position, 1.f);"
                "}");

    bool success = m_material.shader().attach(
                GL_VERTEX_SHADER, vertex_shader);
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

    success = success && m_line_material.shader().attach(
                GL_VERTEX_SHADER, vertex_shader);
    success = success && m_line_material.shader().attach(
                GL_FRAGMENT_SHADER,
                "#version 330\n"
                "out vec4 color;"
                "in vec3 norm;"
                "void main() {"
                "   color = vec4(1, 1, 1, 1);"
                "}");
    success = success && m_line_material.shader().link();

    m_material.shader().bind();

    if (!success) {
        throw std::runtime_error("failed to compile or link shader");
    }

    m_vao = decl.make_vao(m_material.shader(), true);
    m_line_vao = decl.make_vao(m_line_material.shader(), true);

    if (m_chunks.empty()) {
        for (unsigned int i = 0; i < m_xchunks*m_ychunks; i++) {
            m_chunks.emplace_back(new QuadTerrainChunk(m_pos_vbo, m_normal_vbo, m_ibo));
        }
    }
}

void QuadTerrainNode::set_grass_texture(Texture2D *tex)
{
    m_material.attach_texture("grass", tex);
}

void QuadTerrainNode::update()
{
    static const std::array<unsigned int, 4> neighbour_lods({
                                                                1, 1, 1, 1
                                                            });

    log.log(io::LOG_DEBUG, "update()");
    for (unsigned int y = 0; y < m_ychunks; y++) {
        for (unsigned int x = 0; x < m_xchunks; x++) {
            sim::QuadNode *node =
                    m_root->find_node_at(sim::TerrainRect::point_t(x, y),
                                         m_chunk_lod);

            assert(node);
            if (node->size() > m_chunk_lod) {
                log.logf(io::LOG_DEBUG, "chunk %u,%u as plane",
                         x, y);
                m_chunks[y*m_ychunks+x]->update(x*m_chunk_lod,
                                                y*m_chunk_lod,
                                                m_chunk_lod,
                                                node->height());
            } else {
                log.logf(io::LOG_DEBUG, "chunk %u,%u from quadtree",
                         x, y);
                m_chunks[y*m_ychunks+x]->update(m_root, node, 1, neighbour_lods);
            }
        }
    }
}

void QuadTerrainNode::render(RenderContext &context)
{
    for (auto &chunk: m_chunks)
    {
        context.draw_elements_base_vertex(
                    GL_TRIANGLES, *m_vao, m_material,
                    chunk->ibo_alloc(),
                    chunk->base());
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(0., 10.);
    for (auto &chunk: m_chunks)
    {
        context.draw_elements_base_vertex(
                    GL_TRIANGLES, *m_line_vao, m_line_material,
                    chunk->ibo_alloc(),
                    chunk->base());
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void QuadTerrainNode::sync()
{
    log.log(io::LOG_DEBUG, "sync()");
    for (auto &chunk: m_chunks) {
        chunk->release_buffers();
    }
    for (auto &chunk: m_chunks)
    {
        log.log(io::LOG_DEBUG, "synchronizing chunk");
        chunk->mesh_to_buffers();
    }
    m_vao->sync();
}


}
