#include "engine/render/rendergraph.hpp"

namespace engine {

RenderContext::RenderContext():
    m_matrix_ubo(),
    m_model_stack(),
    m_current_transformation(Identity)
{
    m_matrix_ubo.bind();
    m_matrix_ubo.bind_at(MATRIX_BLOCK_UBO_SLOT);
}

void RenderContext::prepare_draw()
{
    m_matrix_ubo.set<2>(m_current_transformation);
    Matrix3f rotational_part = Matrix3f::clip(m_current_transformation);
    invert(rotational_part);
    m_matrix_ubo.set<3>(rotational_part);
    m_matrix_ubo.update_bound();
}

void RenderContext::draw_elements(GLenum primitive,
                                  VAO &with_arrays,
                                  Material &using_material,
                                  IBOAllocation &indicies)
{
    prepare_draw();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements(indicies, primitive);
}

void RenderContext::draw_elements_base_vertex(
        GLenum primitive,
        VAO &with_arrays, Material &using_material,
        IBOAllocation &indicies,
        GLint base_vertex)
{
    prepare_draw();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements_base_vertex(indicies, primitive, base_vertex);
}

void RenderContext::push_transformation(const Matrix4f &mat)
{
    m_model_stack.push_back(m_current_transformation);
    m_current_transformation *= mat;
}

void RenderContext::pop_transformation()
{
    m_current_transformation = m_model_stack.back();
    m_model_stack.pop_back();
}

void RenderContext::reset()
{
    m_model_stack.clear();
    m_current_transformation = Matrix4f(Identity);
    m_matrix_ubo.set<0>(Matrix4f(Identity));
    m_matrix_ubo.set<1>(Matrix4f(Identity));
    m_matrix_ubo.set<2>(Matrix4f(Identity));
    m_matrix_ubo.set<3>(Matrix3f(Identity));
    m_viewpoint = Vector3f(0, 0, 0);
}

Matrix4f RenderContext::projection()
{
    return m_matrix_ubo.get<0>();
}

Matrix4f RenderContext::view()
{
    return m_matrix_ubo.get<1>();
}

void RenderContext::set_projection(const Matrix4f &proj)
{
    m_matrix_ubo.set<0>(proj);
}

void RenderContext::set_view(const Matrix4f &view)
{
    m_matrix_ubo.set<1>(view);
}

void RenderContext::set_viewpoint(const Vector3f &viewpoint)
{
    m_viewpoint = viewpoint;
}

void RenderContext::configure_shader(ShaderProgram &shader)
{
    if (shader.uniform_block_location("MatrixBlock") >= 0) {
        shader.check_uniform_block<MatrixUBO>("MatrixBlock");
        shader.bind_uniform_block(
                    "MatrixBlock",
                    MATRIX_BLOCK_UBO_SLOT);
    }
}

}
