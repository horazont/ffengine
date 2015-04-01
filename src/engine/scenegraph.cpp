#include "scenegraph.h"

#include "gl/ibo.h"

#include "../io/log.h"

namespace engine {
namespace scenegraph {

io::Logger &scenegraph_logger = io::logging().get_logger("engine.scenegraph");


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


Node::Node()
{

}

Node::~Node()
{

}

void Node::advance(TimeInterval)
{

}


Group::Group():
    m_locked_children(),
    m_children(),
    m_to_render()
{

}

void Group::add(std::unique_ptr<Node> &&node)
{
    m_children.push_back(std::move(node));
}

Node &Group::at(const unsigned int index)
{
    return *m_children.at(index).get();
}

Group::iterator Group::erase(iterator iter)
{
    // save the pointer into m_locked_children, in case one of them is
    // currently being rendered
    m_locked_children.emplace_back(std::move(*iter.m_curr));
    return iterator(m_children.erase(iter.m_curr));
}

Group::iterator Group::erase(iterator first, iterator last)
{
    // save the pointers into m_locked_children, see erase() for a why
    std::move(first.m_curr, last.m_curr,
              std::back_inserter(m_locked_children));
    return iterator(m_children.erase(first.m_curr, last.m_curr));
}

Node *Group::operator [](const unsigned int at)
{
    if (at >= m_children.size())
    {
        return nullptr;
    }
    return m_children[at].get();
}

std::tuple<std::unique_ptr<Node>, Group::iterator> Group::pop(Group::iterator iter)
{
    std::unique_ptr<Node> result_value(std::move(*iter.m_curr));
    Group::iterator result_iter(m_children.erase(iter.m_curr));
    return std::make_tuple(std::move(result_value), result_iter);
}

void Group::advance(TimeInterval seconds)
{
    for (auto &child: m_children)
    {
        child->advance(seconds);
    }
}

void Group::render(RenderContext &context)
{
    for (auto child: m_to_render)
    {
        child->render(context);
    }
}

void Group::sync()
{
    m_to_render.clear();
    m_locked_children.clear();
    for (auto &child: m_children)
    {
        m_to_render.push_back(child.get());
        child->sync();
    }
}


InvisibleGroup::InvisibleGroup():
    m_children()
{

}

std::vector<std::unique_ptr<Node> > &InvisibleGroup::children()
{
    return m_children;
}

const std::vector<std::unique_ptr<Node> > &InvisibleGroup::children() const
{
    return m_children;
}

void InvisibleGroup::advance(TimeInterval seconds)
{
    for (auto &child: m_children)
    {
        child->advance(seconds);
    }
}

void InvisibleGroup::render(RenderContext &)
{

}

void InvisibleGroup::sync()
{

}


ParentNode::ParentNode():
    Node(),
    m_locked_child(),
    m_child(),
    m_child_to_render()
{

}

ParentNode::ParentNode(std::unique_ptr<Node> &&child):
    Node(),
    m_locked_child(),
    m_child(std::move(child)),
    m_child_to_render()
{

}

void ParentNode::set_child(std::unique_ptr<Node> &&node)
{
    m_locked_child = std::move(m_child);
    m_child = std::move(node);
}

std::unique_ptr<Node> ParentNode::swap_child(std::unique_ptr<Node> &&node)
{
    std::unique_ptr<Node> result = std::move(m_child);
    m_child = std::move(node);
    return result;
}

void ParentNode::advance(TimeInterval seconds)
{
    if (m_child)
    {
        m_child->advance(seconds);
    }
}

void ParentNode::render(RenderContext &context)
{
    if (m_child_to_render)
    {
        m_child_to_render->render(context);
    }
}

void ParentNode::sync()
{
    m_locked_child = nullptr;
    m_child_to_render = m_child.get();
    m_child_to_render->sync();
}


Transformation::Transformation():
    ParentNode(),
    m_transform(Identity),
    m_render_transform(Identity)
{

}

Transformation::Transformation(std::unique_ptr<Node> &&child):
    ParentNode(std::move(child)),
    m_transform(Identity),
    m_render_transform(Identity)
{

}

void Transformation::render(RenderContext &context)
{
    context.push_transformation(m_render_transform);
    ParentNode::render(context);
    context.pop_transformation();
}

void Transformation::sync()
{
    m_render_transform = m_transform;
    ParentNode::sync();
}


CameraPlaneController::CameraPlaneController():
    m_pos(0, 0),
    m_pos_vel(0, 0),
    m_pos_accel(0, 0),
    m_rot(0, 0),
    m_rot_vel(0, 0),
    m_rot_accel(0, 0),
    m_distance(1),
    m_distance_vel(0),
    m_distance_accel(0)
{

}

void CameraPlaneController::set_pos(const Vector2f &pos, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_pos_vel = Vector2f();
        m_pos_accel = Vector2f();
    }

    m_pos = pos;
}

void CameraPlaneController::set_rot(const Vector2f &rot, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_rot_vel = Vector2f();
        m_rot_accel = Vector2f();
    }

    m_rot = rot;
}

void CameraPlaneController::set_distance(const float distance, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_distance_vel = 0;
        m_distance_accel = 0;
    }

    m_distance = distance;
}

void CameraPlaneController::advance(TimeInterval seconds)
{
    const TimeInterval seconds_sqr = seconds * seconds;

    m_pos += m_pos_accel * seconds_sqr / 2. + m_pos_vel * seconds;
    m_pos_vel += m_pos_accel * seconds;

    m_pos_accel /= 180 * seconds;
    m_pos_vel /= 110 * seconds;

    m_rot += m_rot_accel * seconds_sqr / 2. + m_rot_vel * seconds;
    m_rot_vel += m_rot_accel * seconds;

    m_rot_accel /= 180 * seconds;
    m_rot_vel /= 110 * seconds;
}


Camera::Camera()
{

}

Camera::~Camera()
{

}

void Camera::advance(TimeInterval seconds)
{

}

void Camera::configure_context(RenderContext &context)
{
    context.set_projection(m_render_projection);
    context.set_view(m_render_view);
}


OrthogonalCamera::OrthogonalCamera(
        float viewport_width,
        float viewport_height):
    Camera(),
    m_controller(),
    m_viewport_width(viewport_width),
    m_viewport_height(viewport_height),
    m_znear(0),
    m_zfar(100),
    m_projection(proj_ortho(0, 0, m_viewport_width, m_viewport_height,
                            m_znear, m_zfar))
{

}

void OrthogonalCamera::update_projection()
{
    m_projection = proj_ortho_center(
                0, 0,
                m_viewport_width, m_viewport_height,
                m_znear, m_zfar);
}

void OrthogonalCamera::set_viewport(const float width, const float height)
{
    m_viewport_width = width;
    m_viewport_height = height;
    update_projection();
}

void OrthogonalCamera::set_znear(const float znear)
{
    m_znear = znear;
    update_projection();
}

void OrthogonalCamera::set_zfar(const float zfar)
{
    m_zfar = zfar;
    update_projection();
}

void OrthogonalCamera::advance(TimeInterval seconds)
{
    m_controller.advance(seconds);
}

void OrthogonalCamera::sync()
{
    scenegraph_logger.log(io::LOG_DEBUG, "synchronizing camera");

    // put 0, 0, 0 into the viewports center
    m_render_projection = m_projection;

    const Vector2f &pos = m_controller.pos();
    const Vector2f &rot = m_controller.rot();
    const float distance = m_controller.distance();

    m_render_view = translation4(Vector3f(pos[eX], pos[eY], 0.0f))
            * rotation4(eX, -rot[eX])
            * rotation4(eZ, rot[eY])
            * scale4(Vector3(1, 1, 1)/distance);

    scenegraph_logger.log(io::LOG_DEBUG)
            << "view = " << m_render_view
            << io::submit;
    scenegraph_logger.log(io::LOG_DEBUG)
            << "proj = " << m_render_projection
            << io::submit;
}


PerspectivalCamera::PerspectivalCamera():
    Camera(),
    m_controller(),
    m_viewport_width(0),
    m_viewport_height(0),
    m_znear(1.0),
    m_zfar(100),
    m_fovy(45.0),
    m_projection(Identity)
{

}

void PerspectivalCamera::update_projection()
{
    m_projection = proj_perspective(m_fovy,
                                    m_viewport_width/m_viewport_height,
                                    m_znear, m_zfar);
}

void PerspectivalCamera::set_viewport(const float width, const float height)
{
    m_viewport_width = width;
    m_viewport_height = height;
    update_projection();
}

void PerspectivalCamera::set_znear(const float znear)
{
    m_znear = znear;
    update_projection();
}

void PerspectivalCamera::set_zfar(const float zfar)
{
    m_zfar = zfar;
    update_projection();
}

void PerspectivalCamera::advance(TimeInterval seconds)
{
    m_controller.advance(seconds);
}

void PerspectivalCamera::sync()
{
    scenegraph_logger.log(io::LOG_DEBUG, "synchronizing camera");

    // put 0, 0, 0 into the viewports center
    m_render_projection = m_projection;

    const Vector2f &pos = m_controller.pos();
    const Vector2f &rot = m_controller.rot();
    const float distance = m_controller.distance();

    m_render_view = translation4(Vector3f(pos[eX], pos[eY], 0.f))
            * rotation4(eX, -rot[eX])
            * rotation4(eZ, rot[eY])
            * translation4(Vector3f(0, 0, -distance));

    scenegraph_logger.log(io::LOG_DEBUG)
            << "view = " << m_render_view
            << io::submit;
    scenegraph_logger.log(io::LOG_DEBUG)
            << "proj = " << m_render_projection
            << io::submit;
}


SceneGraph::SceneGraph():
    m_render_context(),
    m_root()
{

}

void SceneGraph::render(Camera &camera)
{
    scenegraph_logger.log(io::LOG_DEBUG, "preparing context...");
    camera.configure_context(m_render_context);
    scenegraph_logger.log(io::LOG_DEBUG)
            << "view = " << m_render_context.view()
            << io::submit;
    scenegraph_logger.log(io::LOG_DEBUG)
            << "proj = " << m_render_context.projection()
            << io::submit;
    m_root.render(m_render_context);
}


}
}
