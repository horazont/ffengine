#ifndef SCC_ENGINE_RENDER_QUADTERRAIN_H
#define SCC_ENGINE_RENDER_QUADTERRAIN_H

#include <unordered_map>

#include <surface_mesh/Surface_mesh.h>

#include "engine/gl/vbo.hpp"
#include "engine/gl/ibo.hpp"

#include "engine/sim/quadterrain.hpp"

#include "engine/render/scenegraph.hpp"

namespace engine {

class QuadTerrainChunk
{
public:
    QuadTerrainChunk(VBO &pos_vbo, VBO &normal_vbo, IBO &ibo);

private:
    VBO &m_pos_vbo;
    VBO &m_normal_vbo;
    IBO &m_ibo;

    VBOAllocation m_pos_alloc;
    VBOAllocation m_normal_alloc;
    IBOAllocation m_ialloc;

    surface_mesh::Surface_mesh m_mesh;
    std::vector<sim::TerrainVector> m_tmp_terrain_vectors;
    std::unordered_map<sim::TerrainVector, surface_mesh::Surface_mesh::Vertex> m_vertex_cache;

protected:
    surface_mesh::Surface_mesh::Vertex add_vertex_cached(const sim::TerrainVector &vec);
    void build_from_quadtree(sim::QuadNode *subtree_root,
                             unsigned int lod,
                             const std::array<unsigned int, 4> &neighbour_lod);
    void make_quad(surface_mesh::Surface_mesh::Vertex v1,
                   surface_mesh::Surface_mesh::Vertex v2,
                   surface_mesh::Surface_mesh::Vertex v3,
                   surface_mesh::Surface_mesh::Vertex v4);
    void make_quad_flipped(surface_mesh::Surface_mesh::Vertex v1,
                           surface_mesh::Surface_mesh::Vertex v2,
                           surface_mesh::Surface_mesh::Vertex v3,
                           surface_mesh::Surface_mesh::Vertex v4);


public:
    /**
     * @brief Update the chunk with geometry from the given QuadTree subtree.
     * @param subtree_root Root node of the subtree to source the geometry from
     * @param lod Level of detail to use; this is the minimum size of quad tree
     * nodes which shall be considered. Note that if anything between that
     * size and the subtree_root is a heightmap node, the full level of detail
     * will be applied for the heightmap node, independent of the lod setting.
     * @param neighbour_lod LOD of the neighbouring chunks. This is used to
     * create smooth edges. If the LOD grid is smaller in the neighbour than
     * locally, the neighbour is responsible for setting up a smooth edge. If
     * the LOD grid is equal or greater than locally, the smooth edge is
     * created by this chunk.
     */
    void update(sim::QuadNode *subtree_root,
                unsigned int lod,
                const std::array<unsigned int, 4> &neighbour_lod);

    /**
     * @brief Update the chunk with a static plane
     * @param x0 X origin of the plane
     * @param y0 Y origin of the plane
     * @param size Size of the plane
     * @param height
     */
    void update(const sim::terrain_coord_t x0,
                const sim::terrain_coord_t y0,
                const sim::terrain_coord_t size,
                const sim::terrain_height_t height);

    void mesh_to_buffers();

    inline unsigned int base()
    {
        assert(m_pos_alloc.base() == m_normal_alloc.base());
        return m_pos_alloc.base();
    }

    inline IBOAllocation &ibo_alloc()
    {
        return m_ialloc;
    }

};

class QuadTerrainNode: public scenegraph::Node
{
public:
    QuadTerrainNode(sim::QuadNode *root, unsigned int chunk_lod = 64);

private:
    const unsigned int m_xchunks;
    const unsigned int m_ychunks;
    const unsigned int m_chunk_lod;
    sim::QuadNode *m_root;
    VBO m_pos_vbo;
    VBO m_normal_vbo;
    IBO m_ibo;
    Material m_material;
    Material m_line_material;
    std::vector<std::unique_ptr<QuadTerrainChunk> > m_chunks;
    std::unique_ptr<VAO> m_vao;
    std::unique_ptr<VAO> m_line_vao;

public:
    void update();
    void set_grass_texture(Texture2D *tex);

public:
    void render(RenderContext &context) override;
    void sync() override;

};

}

#endif // SCC_ENGINE_RENDER_QUADTERRAIN_H

