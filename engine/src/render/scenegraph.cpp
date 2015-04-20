/**********************************************************************
File name: scenegraph.cpp
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
#include "engine/render/scenegraph.hpp"

#include "engine/gl/ibo.hpp"

#include "engine/io/log.hpp"

namespace engine {

static io::Logger &logger = io::logging().get_logger("render.scenegraph");

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

void Group::sync(RenderContext &context)
{
    m_to_render.clear();
    m_locked_children.clear();
    for (auto &child: m_children)
    {
        m_to_render.push_back(child.get());
        child->sync(context);
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

void InvisibleGroup::sync(RenderContext &)
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

void ParentNode::sync(RenderContext &context)
{
    m_locked_child = nullptr;
    m_child_to_render = m_child.get();
    if (m_child_to_render) {
        m_child_to_render->sync(context);
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

void Transformation::sync(RenderContext &context)
{
    m_render_transform = m_transform;
    ParentNode::sync(context);
}


} /* namespace scenegraph */


SceneGraph::SceneGraph():
    m_root()
{

}

void SceneGraph::render(RenderContext &context)
{
    m_root.render(context);
}

void SceneGraph::sync(RenderContext &context)
{
    m_root.sync(context);
}


}
