#ifndef SCC_ENGINE_RENDER_QUADTERRAIN_H
#define SCC_ENGINE_RENDER_QUADTERRAIN_H

#include "engine/gl/vbo.hpp"
#include "engine/gl/ibo.hpp"

#include "engine/sim/quadterrain.hpp"

#include "engine/render/scenegraph.hpp"

namespace engine {

class QuadTerrainChunk
{
public:
    QuadTerrainChunk(VBO &vbo, IBO &ibo);

private:
    VBO &m_vbo;
    IBO &m_ibo;

    VBOAllocation m_valloc;
    IBOAllocation m_ialloc;

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

};

class QuadTerrainNode: public scenegraph::Node
{
public:
    QuadTerrainNode(sim::QuadNode *root, unsigned int chunk_lod = 64);

private:
    sim::QuadNode *m_root;
    unsigned int m_xchunks;
    unsigned int m_ychunks;
    std::vector<std::unique_ptr<QuadTerrainChunk> > m_chunks;

public:


};


void create_geometry(
        sim::QuadNode *root,
        sim::QuadNode *subtree_root,
        unsigned int lod,
        const std::array<unsigned int, 4> &neighbour_lod,
        std::vector<unsigned int> &indicies,
        std::vector<Vector3f> &positions,
        std::vector<Vector3f> &normals,
        std::vector<Vector3f> &tangents);

}

#endif // SCC_ENGINE_RENDER_QUADTERRAIN_H

