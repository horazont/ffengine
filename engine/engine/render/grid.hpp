#ifndef SCC_ENGINE_RENDER_GRID_H
#define SCC_ENGINE_RENDER_GRID_H

#include "engine/render/scenegraph.hpp"

namespace engine {

class GridNode: public scenegraph::Node
{
public:
    GridNode(const unsigned int xcells,
             const unsigned int ycells,
             const float size);

private:
    engine::VBO m_vbo;
    engine::IBO m_ibo;
    engine::Material m_material;
    std::unique_ptr<engine::VAO> m_vao;

    engine::VBOAllocation m_vbo_alloc;
    engine::IBOAllocation m_ibo_alloc;

public:
    void render(RenderContext &context) override;
    void sync(Scene &scene) override;

};

}

#endif // SCC_ENGINE_RENDER_GRID_H

