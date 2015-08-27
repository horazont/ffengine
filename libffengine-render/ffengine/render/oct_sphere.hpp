#ifndef SCC_ENGINE_RENDER_OCT_SPHERE_H
#define SCC_ENGINE_RENDER_OCT_SPHERE_H

#include "ffengine/render/scenegraph.hpp"

namespace engine {

class OctSphere: public scenegraph::OctNode, public RenderableOctreeObject
{
public:
    OctSphere(ArrayDeclaration &arrays, VAO &vao, Material &mat, float radius);

private:
    Vector3f m_origin;
    float m_radius;

    Material &m_material;
    VAO &m_vao;

    VBOAllocation m_vbo_alloc;
    IBOAllocation m_ibo_alloc;

public:
    void render(RenderContext &context) override;
    void sync(RenderContext &context, ffe::Octree &octree,
              scenegraph::OctContext &positioning) override;

};

}

#endif
