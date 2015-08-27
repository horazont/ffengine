/**********************************************************************
File name: rendergraph.cpp
This file is part of: SCC (working title)

LICENSE

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

FEEDBACK & QUESTIONS

For feedback and questions about SCC please e-mail one of the authors named in
the AUTHORS file.
**********************************************************************/
#include "ffengine/render/rendergraph.hpp"

#include "ffengine/render/camera.hpp"
#include "ffengine/render/scenegraph.hpp"

namespace engine {

static io::Logger &logger = io::logging().get_logger("render.rendergraph");

RenderContext::RenderContext(SceneGraph &scenegraph, Camera &camera):
    m_scenegraph(scenegraph),
    m_camera(camera),
    m_render_viewpoint(0, 0, 0),
    m_render_view(Identity),
    m_matrix_ubo(),
    m_model_stack(),
    m_current_transformation(Identity)
{

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
    raise_last_gl_error();
    prepare_draw();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements(indicies, primitive);
    raise_last_gl_error();
}

void RenderContext::draw_elements_less(GLenum primitive,
                                       VAO &with_arrays,
                                       Material &using_material,
                                       IBOAllocation &indicies,
                                       unsigned int nmax)
{
    raise_last_gl_error();
    prepare_draw();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements(indicies, primitive, nmax);
    raise_last_gl_error();
}

void RenderContext::draw_elements_base_vertex(
        GLenum primitive,
        VAO &with_arrays, Material &using_material,
        IBOAllocation &indicies,
        GLint base_vertex)
{
    raise_last_gl_error();
    prepare_draw();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements_base_vertex(indicies, primitive, base_vertex);
    raise_last_gl_error();
}

void RenderContext::draw_elements_base_vertex_less(
        GLenum primitive,
        VAO &with_arrays, Material &using_material,
        IBOAllocation &indicies,
        GLint base_vertex,
        unsigned int nmax)
{
    raise_last_gl_error();
    prepare_draw();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements_base_vertex(indicies, primitive, base_vertex, nmax);
    raise_last_gl_error();
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

void RenderContext::start()
{
    m_inv_matrix_ubo.bind_at(INV_MATRIX_BLOCK_UBO_SLOT);
    m_matrix_ubo.bind();
    m_matrix_ubo.bind_at(MATRIX_BLOCK_UBO_SLOT);
}

void RenderContext::set_viewport_size(GLsizei viewport_width,
                                      GLsizei viewport_height)
{
    m_viewport_width = viewport_width;
    m_viewport_height = viewport_height;
}

inline Plane transform_plane(Matrix4f proj, Matrix4f view, Matrix4f inv_view, Vector4f hom)
{
    Vector4f unprojected = proj * hom;
    Vector3f normal(unprojected[eX], unprojected[eY], unprojected[eZ]);
    Vector3f origin = normal*unprojected[eZ];
    normal = Vector3f(view * Vector4f(normal, 0)).normalized();
    origin = Vector3f(inv_view * Vector4f(origin, 1));
    return Plane(origin, normal);
}

void RenderContext::sync()
{
    m_render_view = m_camera.render_view();
    Matrix4f inv_view = m_camera.render_inv_view();
    m_render_viewpoint = Vector3f(inv_view * Vector4f(0, 0, 0, 1));

    std::tie(m_matrix_ubo.get_ref<0>(),
             m_inv_matrix_ubo.get_ref<0>()) = m_camera.render_projection(
                m_viewport_width,
                m_viewport_height);
    m_matrix_ubo.set<1>(m_render_view);
    m_inv_matrix_ubo.set<1>(inv_view);
    m_inv_matrix_ubo.bind();
    m_inv_matrix_ubo.update_bound();

    Matrix4f projview = (m_matrix_ubo.get_ref<0>() * m_render_view).transposed();

    m_frustum[0] = Plane::from_frustum_matrix(projview * Vector4f(1, 0, 0, 1));
    m_frustum[1] = Plane::from_frustum_matrix(projview * Vector4f(-1, 0, 0, 1));
    m_frustum[2] = Plane::from_frustum_matrix(projview * Vector4f(0, 1, 0, 1));
    m_frustum[3] = Plane::from_frustum_matrix(projview * Vector4f(0, -1, 0, 1));
    m_frustum[4] = Plane::from_frustum_matrix(projview * Vector4f(0, 0, 1, 1));
    m_frustum[5] = Plane::from_frustum_matrix(projview * Vector4f(0, 0, -1, 1));

    m_scenegraph.sync(*this);
}

void RenderContext::configure_shader(ShaderProgram &shader)
{
    if (shader.uniform_block_location("MatrixBlock") >= 0) {
        shader.check_uniform_block<MatrixUBO>("MatrixBlock");
        shader.bind_uniform_block(
                    "MatrixBlock",
                    MATRIX_BLOCK_UBO_SLOT);
    }
    if (shader.uniform_block_location("InvMatrixBlock") >= 0) {
        shader.check_uniform_block<InvMatrixUBO>("InvMatrixBlock");
        shader.bind_uniform_block(
                    "InvMatrixBlock",
                    INV_MATRIX_BLOCK_UBO_SLOT);
    }
}


RenderNode::RenderNode(RenderTarget &target):
    m_target(target)
{

}

RenderNode::~RenderNode()
{

}


BlitNode::BlitNode(RenderTarget &src, RenderTarget &dest):
    RenderNode(dest),
    m_src(src)
{

}

void BlitNode::render()
{
    m_src.bind(RenderTarget::Usage::READ);
    m_target.bind(RenderTarget::Usage::DRAW);
    glBlitFramebuffer(0, 0, m_src.width(), m_src.height(),
                      0, 0, m_target.width(), m_target.height(),
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);
    raise_last_gl_error();
}

void BlitNode::sync()
{

}


SceneRenderNode::SceneRenderNode(RenderTarget &target,
                                 SceneGraph &scenegraph,
                                 Camera &camera):
    RenderNode(target),
    m_context(scenegraph, camera),
    m_clear_mask(0),
    m_clear_colour(1, 1, 1, 1)
{

}

void SceneRenderNode::set_clear_mask(GLbitfield mask)
{
    m_clear_mask = mask;
}

void SceneRenderNode::set_clear_colour(const Vector4f &colour)
{
    m_clear_colour = colour;
}

void SceneRenderNode::render()
{
    m_target.bind(RenderTarget::Usage::DRAW);
    if (m_clear_mask) {
        glClearColor(m_clear_colour[eX],
                     m_clear_colour[eY],
                     m_clear_colour[eZ],
                     m_clear_colour[eW]);
        glClear(m_clear_mask);
    }
    m_context.start();
    m_context.scenegraph().render(m_context);
}

void SceneRenderNode::sync()
{
    m_context.set_viewport_size(m_target.width(),
                                m_target.height());
    m_context.sync();
}



RenderGraph::RenderGraph()
{

}

bool RenderGraph::resort()
{
    m_ordered.clear();

    std::vector<RenderNode*> no_incoming_edges;
    std::unordered_map<RenderNode*, std::vector<RenderNode*> > tmp_deps;
    no_incoming_edges.reserve(m_nodes.size());

    for (auto &node_ptr: m_nodes) {
        RenderNode &node = *node_ptr;
        if (node.dependencies().empty()) {
            no_incoming_edges.push_back(&node);
        } else {
            std::vector<RenderNode*> &vector = tmp_deps[&node];
            vector.reserve(node.dependencies().size());
            std::copy(node.dependencies().begin(),
                      node.dependencies().end(),
                      std::back_inserter(vector));
        }
    }

    while (!no_incoming_edges.empty()) {
        RenderNode *node = no_incoming_edges.back();
        m_ordered.push_back(node);
        no_incoming_edges.pop_back();

        auto iter = tmp_deps.begin();
        while (iter != tmp_deps.end())
        {
            auto depiter = std::find(iter->second.begin(),
                                     iter->second.end(),
                                     node);
            if (depiter == iter->second.end()) {
                ++iter;
                continue;
            }

            iter->second.erase(depiter);
            if (iter->second.empty()) {
                no_incoming_edges.push_back(iter->first);
                iter = tmp_deps.erase(iter);
                continue;
            }

            ++iter;
        }
    }

    if (tmp_deps.size() > 0) {
        m_ordered.clear();
        logger.logf(io::LOG_ERROR, "render graph has cycles; won’t render this shit");
        return false;
    }

    return true;
}

void RenderGraph::render()
{
    for (auto node: m_render_order) {
        node->render();
    }
}

void RenderGraph::sync()
{
    m_locked_nodes.clear();
    m_render_order = m_ordered;
    for (RenderNode *&node: m_render_order)
    {
        node->sync();
    }
}

}
