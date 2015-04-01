#ifndef SCC_ENGINE_RENDER_TERRAIN_H
#define SCC_ENGINE_RENDER_TERRAIN_H

#include "engine/gl/vbo.hpp"
#include "engine/gl/ibo.hpp"
#include "engine/gl/vao.hpp"

#include "engine/render/scenegraph.hpp"

#include "engine/sim/terrain.hpp"

namespace engine {

class Terrain: public scenegraph::Node
{
public:
    static const unsigned int CHUNK_SIZE;
    static const unsigned int CHUNK_VERTICIES;
    static const VBOFormat VBO_FORMAT;

public:
    Terrain(sim::Terrain &src);

private:
    const unsigned int m_xchunks;
    const unsigned int m_ychunks;
    sim::Terrain &m_source;
    VBO m_vbo;
    IBO m_ibo;
    IBOAllocation m_chunk_indicies;
    Material m_material;
    std::vector<VBOAllocation> m_chunks;
    std::unique_ptr<VAO> m_vao;

protected:
    void sync_from_sim();
    void sync_chunk_from_sim(const unsigned int xchunk,
                             const unsigned int ychunk,
                             VBOAllocation &vbo_alloc);

public:
    void set_grass_texture(Texture2D *tex);

public:
    void render(RenderContext &context) override;
    void sync() override;

};

}

#endif
