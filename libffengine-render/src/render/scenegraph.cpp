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
#include "ffengine/render/scenegraph.hpp"

#include "ffengine/gl/ibo.hpp"

#include "ffengine/io/log.hpp"

// #define TIMELOG_SCENEGRAPH

#ifdef TIMELOG_SCENEGRAPH
#include <chrono>
typedef std::chrono::steady_clock timelog_clock;
#define ms_cast(x) std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1000> > >(x)
#endif


namespace ffe {

static io::Logger &logger = io::logging().get_logger("render.scenegraph");


/* engine::RenderableOctreeObject */

void RenderableOctreeObject::render(RenderContext &)
{

}


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

/* engine::scenegraph::OctContext */

OctContext::OctContext()
{
    reset();
}

void OctContext::push_translation(const Vector3f &d)
{
    const Quaternionf &curr_rot = get_orientation();
    // std::cout << "push_translation(" << d << ");" << std::endl;
    m_transformation_stack.emplace_back(
                curr_rot,
                get_origin() + curr_rot.rotate(d)
                );
    // std::cout << " -> " << get_origin() << std::endl;
}

void OctContext::push_rotation(const Quaternionf &q)
{
    m_transformation_stack.emplace_back(
                get_orientation() * q,
                get_origin());
}

void OctContext::pop_transform()
{
    m_transformation_stack.pop_back();
    /*std::cout << "pop_transform()" << std::endl;
    std::cout << " -> " << get_origin() << std::endl;*/
}

void OctContext::reset()
{
    m_transformation_stack.clear();
    m_transformation_stack.emplace_back(
                Quaternionf(),
                Vector3f()
                );
}

/* engine::scenegraph::OctNode */

OctNode::~OctNode()
{

}

void OctNode::advance(TimeInterval)
{

}

void OctNode::sync(RenderContext &, ffe::Octree &, OctContext &)
{

}

/* engine::scenegraph::OctGroup */

OctGroup::OctGroup():
    OctNode()
{

}

void OctGroup::add(std::unique_ptr<OctNode> &&node)
{
    m_children.emplace_back(std::move(node));
}

OctNode &OctGroup::at(const unsigned int index)
{
    return *m_children.at(index).get();
}

OctGroup::iterator OctGroup::erase(iterator iter)
{
    m_locked_children.emplace_back(std::move(*iter.m_curr));
    return iterator(m_children.erase(iter.m_curr));
}

OctGroup::iterator OctGroup::erase(iterator first, iterator last)
{
    std::move(first.m_curr, last.m_curr,
              std::back_inserter(m_locked_children));
    return iterator(m_children.erase(first.m_curr, last.m_curr));
}

OctNode *OctGroup::operator [](const unsigned int at)
{
    if (at >= m_children.size())
    {
        return nullptr;
    }
    return m_children[at].get();
}

std::tuple<std::unique_ptr<OctNode>, OctGroup::iterator> OctGroup::pop(
        OctGroup::iterator iter)
{
    std::unique_ptr<OctNode> result_value(std::move(*iter.m_curr));
    OctGroup::iterator result_iter(m_children.erase(iter.m_curr));
    return std::make_tuple(std::move(result_value), result_iter);
}

void OctGroup::advance(TimeInterval seconds)
{
    for (auto &child: m_children) {
        child->advance(seconds);
    }
}

void OctGroup::sync(RenderContext &context, ffe::Octree &octree, OctContext &positioning)
{
    m_locked_children.clear();
    for (auto &child: m_children) {
        child->sync(context, octree, positioning);
    }
}

/* engine::scenegraph::OctParentNode */

std::unique_ptr<OctNode> OctParentNode::swap_child(std::unique_ptr<OctNode> &&node)
{
    std::unique_ptr<OctNode> result = std::move(m_child);
    m_child = std::move(node);
    return result;
}

void OctParentNode::set_child(std::unique_ptr<OctNode> &&node)
{
    m_locked_child = std::move(m_child);
    m_child = std::move(node);
}

void OctParentNode::advance(TimeInterval seconds)
{
    if (m_child)
    {
        m_child->advance(seconds);
    }
}

void OctParentNode::sync(RenderContext &context,
                         ffe::Octree &octree,
                         OctContext &positioning)
{
    m_locked_child = nullptr;
    if (m_child) {
        m_child->sync(context, octree, positioning);
    }
}

/* engine::scenegraph::OctRotation */

OctRotation::OctRotation(const Quaternionf &q):
    OctParentNode(),
    m_rotation(q)
{

}

void OctRotation::sync(RenderContext &context, ffe::Octree &octree,
                       OctContext &positioning)
{
    positioning.push_rotation(m_rotation);
    OctParentNode::sync(context, octree, positioning);
    positioning.pop_transform();
}

/* engine::scenegraph::OctTranslation */

OctTranslation::OctTranslation(const Vector3f &d):
    OctParentNode(),
    m_translation(d)
{

}

void OctTranslation::sync(RenderContext &context, ffe::Octree &octree,
                          OctContext &positioning)
{
    positioning.push_translation(m_translation);
    OctParentNode::sync(context, octree, positioning);
    positioning.pop_transform();
}

/* engine::scenegraph::OctreeGroup */

void OctreeGroup::advance(TimeInterval seconds)
{
    m_root.advance(seconds);
}

void OctreeGroup::sync(RenderContext &context)
{
#ifdef TIMELOG_SCENEGRAPH
    timelog_clock::time_point t0 = timelog_clock::now();
    timelog_clock::time_point t_sync, t_select, t_push;
#endif

    m_positioning.reset();
    m_root.sync(context, m_octree, m_positioning);

    m_hitset.clear();

#ifdef TIMELOG_SCENEGRAPH
    t_sync = timelog_clock::now();
#endif

    m_octree.select_nodes_by_frustum(context.frustum(), m_hitset);

#ifdef TIMELOG_SCENEGRAPH
    t_select = timelog_clock::now();
#endif

    m_to_render.clear();
    for (const ffe::OctreeNode *const node: m_hitset)
    {
        /*std::cout << "node " << node << " in frustum" << std::endl;*/
        for (auto iter = node->cbegin();
             iter != node->cend();
             ++iter)
        {
            ffe::OctreeObject *const obj = *iter;
            /* std::cout << "object " << obj << " in frustum" << std::endl; */
#ifdef NDEBUG
            m_to_render.push_back(static_cast<RenderableOctreeObject*>(obj));
#else
            auto renderable = dynamic_cast<RenderableOctreeObject*>(obj);
            assert(renderable);
            m_to_render.push_back(renderable);
#endif
        }
    }
    m_selected_objects = m_to_render.size();

#ifdef TIMELOG_SCENEGRAPH
    t_push = timelog_clock::now();
    logger.logf(io::LOG_DEBUG, "sync:");
    logger.logf(io::LOG_DEBUG, "  t_sync   = %.2f ms", ms_cast(t_sync - t0).count());
    logger.logf(io::LOG_DEBUG, "  t_select = %.2f ms", ms_cast(t_select - t_sync).count());
    logger.logf(io::LOG_DEBUG, "  t_push   = %.2f ms", ms_cast(t_push - t_select).count());
#endif
}

void OctreeGroup::render(RenderContext &context)
{
    for (RenderableOctreeObject *renderable: m_to_render)
    {
        renderable->render(context);
    }
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
