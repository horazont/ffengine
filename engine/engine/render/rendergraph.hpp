#ifndef SCC_ENGINE_RENDERGRAPH_H
#define SCC_ENGINE_RENDERGRAPH_H

#include <stack>

#include "engine/gl/vao.hpp"
#include "engine/gl/material.hpp"

namespace engine {

class DrawRequest
{
public:
    DrawRequest(VAO &arrays, Material &material);

protected:
    VAO &m_arrays;
    Material &m_material;

public:
    VAO &arrays();
    Material &material();

};


class BasicDrawRequest
{
public:
    BasicDrawRequest(VAO &arrays, Material &material);
};


class RenderContext
{
public:
    static constexpr GLint MATRIX_BLOCK_UBO_SLOT = 0;
    typedef UBO<Matrix4f, Matrix4f, Matrix4f, Matrix3f> MatrixUBO;

public:
    RenderContext();

private:
    MatrixUBO m_matrix_ubo;
    std::stack<Matrix4f> m_model_stack;
    Matrix4 m_current_transformation;

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

public:
    Matrix4f projection();
    Matrix4f view();

    void set_projection(const Matrix4f &proj);
    void set_view(const Matrix4f &view);

};

}

#endif
