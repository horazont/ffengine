#include "scenegraph.h"

#include "gl/ibo.h"

namespace engine {
namespace scenegraph {


RenderContext::RenderContext(RenderContext::MatrixUBO &matrix_ubo):
    m_matrix_ubo(matrix_ubo),
    m_model_stack(),
    m_current_transformation(Identity)
{

}

void RenderContext::draw_elements(GLenum primitive,
                                  VAO &with_arrays,
                                  Material &using_material,
                                  IBOAllocation &indicies)
{
    m_matrix_ubo.set<2>(m_current_transformation);
    m_matrix_ubo.sync();
    m_matrix_ubo.bind_at(0);
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


}
}
