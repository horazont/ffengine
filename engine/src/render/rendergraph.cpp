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

void RenderContext::draw_elements(GLenum primitive,
                                  VAO &with_arrays,
                                  Material &using_material,
                                  IBOAllocation &indicies)
{
    m_matrix_ubo.set<2>(m_current_transformation);
    Matrix3f rotational_part = Matrix3f::clip(m_current_transformation);
    inverse(rotational_part);
    m_matrix_ubo.set<3>(rotational_part);
    m_matrix_ubo.update_bound();
    with_arrays.bind();
    using_material.shader().bind();
    ::engine::draw_elements(indicies, primitive);
}

void RenderContext::push_transformation(const Matrix4f &mat)
{
    m_model_stack.push(m_current_transformation);
    m_current_transformation *= mat;
}

void RenderContext::pop_transformation()
{
    m_current_transformation = m_model_stack.top();
    m_model_stack.pop();
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

}
