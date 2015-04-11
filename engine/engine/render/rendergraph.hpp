#ifndef SCC_ENGINE_RENDERGRAPH_H
#define SCC_ENGINE_RENDERGRAPH_H

#include <vector>

#include "engine/gl/vao.hpp"
#include "engine/gl/material.hpp"

namespace engine {

/**
 * Track the environment in which a render takes place.
 */
class RenderContext
{
public:
    static constexpr GLint MATRIX_BLOCK_UBO_SLOT = 0;
    typedef UBO<Matrix4f, Matrix4f, Matrix4f, Matrix3f> MatrixUBO;

public:
    RenderContext();

private:
    MatrixUBO m_matrix_ubo;
    std::vector<Matrix4f> m_model_stack;
    Matrix4f m_current_transformation;

    Vector3f m_viewpoint;

protected:
    void prepare_draw();

public:
    void draw_elements(GLenum primitive,
                       VAO &with_arrays, Material &using_material,
                       IBOAllocation &indicies);
    void draw_elements_base_vertex(GLenum primitive,
                                   VAO &with_arrays, Material &using_material,
                                   IBOAllocation &indicies,
                                   GLint base_vertex);

    void pop_transformation();

    void push_transformation(const Matrix4f &mat);

    void reset();

public:
    Matrix4f projection();
    Matrix4f view();

    inline const Vector3f &viewpoint() const
    {
        return m_viewpoint;
    }

    void set_projection(const Matrix4f &proj);
    void set_view(const Matrix4f &view);
    void set_viewpoint(const Vector3f &viewpoint);

public:
    /**
     * Configure a linked shader for use with RenderContext instances.
     *
     * This introspects the shaders uniform blocks and binds the UBOs of
     * RenderContexts which match the declarations to the corresponding
     * variables.
     *
     * @param shader A shader program to configure.
     */
    static void configure_shader(ShaderProgram &shader);

};


}

#endif
