#include "engine/render/rendergraph.hpp"

#include "engine/render/camera.hpp"
#include "engine/render/scenegraph.hpp"

namespace engine {

static io::Logger &logger = io::logging().get_logger("engine.render.rendergraph");

Scene::Scene(SceneGraph &scenegraph, Camera &camera):
    m_scenegraph(scenegraph),
    m_camera(camera),
    m_render_viewpoint(0, 0, 0),
    m_render_view(Identity)
{

}

void Scene::sync()
{
    m_render_view = m_camera.render_view();
    m_render_viewpoint = Vector3f(m_camera.render_inv_view() * Vector4f(0, 0, 0, 1));
    m_scenegraph.sync(*this);
}


RenderContext::RenderContext(Scene &scene):
    m_scene(scene),
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
    raise_last_gl_error();
    prepare_draw();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements(indicies, primitive);
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
    m_matrix_ubo.dump_local_as_floats();
    with_arrays.bind();
    using_material.bind();
    ::engine::draw_elements_base_vertex(indicies, primitive, base_vertex);
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

void RenderContext::set_viewport_size(GLsizei viewport_width,
                                      GLsizei viewport_height)
{
    m_viewport_width = viewport_width;
    m_viewport_height = viewport_height;
}

void RenderContext::sync()
{
    m_matrix_ubo.set<0>(m_scene.camera().render_projection(
                            m_viewport_width,
                            m_viewport_height));
    m_matrix_ubo.set<1>(m_scene.view());
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


SceneRenderNode::SceneRenderNode(RenderTarget &target, Scene &scene):
    RenderNode(target),
    m_scene(scene),
    m_context(scene),
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
    m_scene.scenegraph().render(m_context);
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

Scene &RenderGraph::new_scene(SceneGraph &scenegraph, Camera &camera)
{
    Scene *scene = new Scene(scenegraph, camera);
    m_scenes.emplace_back(scene);
    return *scene;
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
    m_locked_scenes.clear();
    for (auto &scene: m_scenes)
    {
        scene->sync();
    }

    m_locked_nodes.clear();
    m_render_order = m_ordered;
    for (RenderNode *&node: m_render_order)
    {
        node->sync();
    }
}

}
