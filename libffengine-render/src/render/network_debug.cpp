/**********************************************************************
File name: network_debug.cpp
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
#include "ffengine/render/network_debug.hpp"

#include "ffengine/render/renderpass.hpp"
#include "ffengine/math/aabb.hpp"


namespace ffe {

DebugNode::DebugNode(const sim::object_ptr<sim::PhysicalNode> &node):
    m_node(node)
{
    update_bounds(Sphere{m_node->position(), 5.f});
}

void DebugNode::prepare(RenderContext &)
{
    // rendering is handled by DebugNodes
}

void DebugNode::render(RenderContext &)
{
    // rendering is handled by DebugNodes
}




DebugNodes::DebugNodes(Octree &octree, Material &material):
    OctNode(octree),
    m_material(material)
{
    octree.insert_object(this);
}

void DebugNodes::cleanup_dead()
{
    auto iter = m_nodes.begin();
    while (iter != m_nodes.end())
    {
        if (!(*iter).first) {
            iter = m_nodes.erase(iter);
            m_changed = true;
            continue;
        }
        ++iter;
    }
}

void DebugNodes::register_node(const sim::object_ptr<sim::PhysicalNode> &node)
{
    if (!node) {
        return;
    }
    auto octree_object = std::make_unique<DebugNode>(node);
    m_octree.insert_object(octree_object.get());
    m_nodes.emplace(node, std::move(octree_object));
    m_changed = true;
}

void DebugNodes::prepare(RenderContext &)
{

}

void DebugNodes::render(RenderContext &context)
{
    if (m_ibo_alloc) {
        context.render_all(AABB(), GL_POINTS, m_material, m_ibo_alloc, m_vbo_alloc);
    }
}

void DebugNodes::sync(scenegraph::OctContext &)
{
    cleanup_dead();
    if ((!m_ibo_alloc || m_ibo_alloc.length() != m_nodes.size()) && !m_nodes.empty())
    {
        m_ibo_alloc = nullptr;
        m_vbo_alloc = nullptr;

        m_ibo_alloc = m_material.ibo().allocate(m_nodes.size());
        m_vbo_alloc = m_material.vbo().allocate(m_nodes.size());

        uint16_t *ibo = m_ibo_alloc.get();
        for (unsigned int i = 0; i < m_nodes.size(); ++i) {
            *ibo++ = i;
        }
        m_ibo_alloc.mark_dirty();
    }

    if (!m_nodes.empty() && m_changed) {
        AABB boundingbox;

        auto positions = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        unsigned int vertex_counter = 0;
        for (auto &node: m_nodes)
        {
            const Vector3f &pos = node.first->position();
            boundingbox = ::bounds(boundingbox, AABB{pos, pos});
            positions[vertex_counter++] = pos;
        }
        m_vbo_alloc.mark_dirty();
        m_material.sync_buffers();

        update_bounds(Sphere{(boundingbox.max + boundingbox.min) / 2.f,
                             (boundingbox.max - boundingbox.min).length()});
    }

    m_changed = false;
}


/* ffe::DebugEdgeBundle */

DebugEdgeBundle::DebugEdgeBundle(Octree &octree, Material &material,
                                 const sim::PhysicalEdgeBundle &bundle):
    OctNode(octree),
    m_material(material)
{
    unsigned int vertex_count = 0;
    unsigned int segment_count = 0;
    Vector3f min(std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max()),
            max(std::numeric_limits<float>::min(),
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::min());
    std::vector<std::vector<std::tuple<Vector3f, Vector2f>> > lines;
    for (const sim::PhysicalEdge &edge: bundle)
    {
        lines.emplace_back();
        std::vector<std::tuple<Vector3f, Vector2f>> &line = lines.back();
        vertex_count += edge.segments().size()+1;
        segment_count += edge.segments().size();

        for (const sim::PhysicalEdgeSegment &segment: edge.segments())
        {
            Vector2f direction(Vector2f(segment.direction).normalized());
            if (edge.reversed()) {
                direction = -direction;
            }
            line.emplace_back(segment.start, direction);
        }

        Vector2f direction(Vector2f(edge.segments().back().direction).normalized());
        if (edge.reversed()) {
            direction = -direction;
        }
        line.emplace_back(edge.segments().back().start + edge.segments().back().direction,
                          direction);

        std::cout << "edge " << std::get<0>(line.front()) << " -- " << std::get<0>(line.back()) << std::endl;
    }

    m_ibo_alloc = m_material.ibo().allocate(segment_count*2);
    m_vbo_alloc = m_material.vbo().allocate(vertex_count);

    uint16_t *dest = m_ibo_alloc.get();
    unsigned int vertex_counter = 0;
    auto positions = VBOSlice<Vector3f>(m_vbo_alloc, 0);
    auto directions = VBOSlice<Vector2f>(m_vbo_alloc, 1);
    for (const std::vector<std::tuple<Vector3f, Vector2f> > &line: lines)
    {
        for (unsigned int i = 0; i < line.size()-1; ++i)
        {
            *dest++ = vertex_counter + i;
            *dest++ = vertex_counter + i+1;
        }

        for (auto &vertex: line)
        {
            const Vector3f &position = std::get<0>(vertex);
            min = Vector3f(std::min(min[eX], position[eX]),
                           std::min(min[eY], position[eY]),
                           std::min(min[eZ], position[eZ]));
            max = Vector3f(std::max(max[eX], position[eX]),
                           std::max(max[eY], position[eY]),
                           std::max(max[eZ], position[eZ]));
            positions[vertex_counter] = position;
            directions[vertex_counter] = std::get<1>(vertex);
            vertex_counter++;
        }
    }
    m_ibo_alloc.mark_dirty();
    m_vbo_alloc.mark_dirty();

    m_material.sync_buffers();

    std::cout << "bundle has " << vertex_count << " vertices in "
              << segment_count << " lines spanning from "
              << min << " to " << max << std::endl;

    update_bounds(Sphere{(max + min)/2.f, (max - min).length() / 2.f});

    octree.insert_object(this);
}

void DebugEdgeBundle::prepare(RenderContext &)
{

}

void DebugEdgeBundle::render(RenderContext &context)
{
    context.render_all(AABB{}, GL_LINES, m_material, m_ibo_alloc, m_vbo_alloc);
}

void DebugEdgeBundle::sync(scenegraph::OctContext &)
{

}

}
