#include "engine/render/scenegraph.hpp"

#include "engine/gl/ibo.hpp"

#include "engine/io/log.hpp"

namespace engine {

io::Logger &scenegraph_logger = io::logging().get_logger("engine.render.scenegraph");

namespace scenegraph {

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
    if (m_child_to_render) {
        m_child_to_render->sync();
    }
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


} /* namespace scenegraph */


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
