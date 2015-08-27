#include "ffengine/render/oct_sphere.hpp"

namespace engine {

OctSphere::OctSphere(ArrayDeclaration &arrays, VAO &vao, Material &mat, float radius):
    OctNode(),
    RenderableOctreeObject(),
    m_radius(radius),
    m_material(mat),
    m_vao(vao),
    m_vbo_alloc(arrays.get_attribute("position").vbo.allocate(6)),
    m_ibo_alloc(arrays.ibo()->allocate(8*3))
{
    {
        auto slice = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        slice[0] = Vector3f(-m_radius, 0, 0);
        slice[1] = Vector3f(m_radius, 0, 0);
        slice[2] = Vector3f(0, -m_radius, 0);
        slice[3] = Vector3f(0, m_radius, 0);
        slice[4] = Vector3f(0, 0, -m_radius);
        slice[5] = Vector3f(0, 0, m_radius);
        m_vbo_alloc.mark_dirty();
    }

    uint16_t base = m_vbo_alloc.base();
    {
        uint16_t *dest = m_ibo_alloc.get();
        *dest++ = base+0;
        *dest++ = base+4;
        *dest++ = base+2;

        *dest++ = base+0;
        *dest++ = base+2;
        *dest++ = base+5;

        *dest++ = base+0;
        *dest++ = base+3;
        *dest++ = base+4;

        *dest++ = base+0;
        *dest++ = base+5;
        *dest++ = base+3;

        *dest++ = base+1;
        *dest++ = base+2;
        *dest++ = base+4;

        *dest++ = base+1;
        *dest++ = base+5;
        *dest++ = base+2;

        *dest++ = base+1;
        *dest++ = base+4;
        *dest++ = base+3;

        *dest++ = base+1;
        *dest++ = base+3;
        *dest++ = base+5;
        m_ibo_alloc.mark_dirty();
    }
}

void OctSphere::render(RenderContext &context)
{
    context.push_transformation(translation4(m_origin));
    /*std::cout << "rendering at " << m_origin << std::endl; */
    context.draw_elements(GL_TRIANGLES, m_vao, m_material, m_ibo_alloc);
    context.pop_transformation();
}

void OctSphere::sync(RenderContext &, ffe::Octree &octree,
                     scenegraph::OctContext &positioning)
{
    if (this->octree() != &octree) {
        if (this->octree()) {
            this->octree()->remove_object(this);
        }
    }

    m_origin = positioning.get_origin();
    /* std::cout << m_origin << std::endl; */
    update_bounds(Sphere{m_origin, m_radius});

    if (this->octree() != &octree) {
        std::cout << "inserting at " << m_origin << std::endl;
        octree.insert_object(this);
    }
}

}
