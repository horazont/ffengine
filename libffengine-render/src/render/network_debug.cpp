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
                                 sim::SignalQueue &queue,
                                 const sim::PhysicalGraph &graph,
                                 const sim::object_ptr<sim::PhysicalEdgeBundle> &bundle):
    OctNode(octree),
    m_bundle(bundle),
    m_material(material),
    m_reshaped(true),
    m_reshaped_conn(queue.connect_queued(
                        graph.edge_bundle_reshaped(),
                        std::bind(&DebugEdgeBundle::on_reshaped,
                                  this,
                                  std::placeholders::_1)))
{

}

DebugEdgeBundle::~DebugEdgeBundle()
{
    if (!m_bundle) {
        m_reshaped_conn.release();
    }
}

void DebugEdgeBundle::on_reshaped(sim::object_ptr<sim::PhysicalEdgeBundle> bundle)
{
    if (!m_bundle) {
        m_reshaped_conn.release();
        return;
    }

    if (m_bundle.get() != bundle.get()) {
        return;
    }

    std::cout << bundle.get() << " reshaped!!!" << std::endl;
    m_reshaped = true;
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
    if (!m_reshaped || !m_bundle) {
        return;
    }
    std::cout << m_bundle.get() << " reshaping!!!" << std::endl;
    m_reshaped = false;

    unsigned int vertex_count = 0;
    unsigned int segment_count = 0;
    Vector3f min(std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max(),
                 std::numeric_limits<float>::max()),
            max(std::numeric_limits<float>::min(),
                std::numeric_limits<float>::min(),
                std::numeric_limits<float>::min());
    std::vector<std::vector<std::tuple<Vector3f, Vector2f>> > lines;
    for (const sim::PhysicalEdge &edge: *m_bundle)
    {
        lines.emplace_back();
        std::vector<std::tuple<Vector3f, Vector2f>> &line = lines.back();

        const float reverse_factor = (edge.reversed() ? -1 : 1);

        {
            const sim::PhysicalEdgeSegment &first = edge.segments()[edge.first_non_cut_segment()];
            line.emplace_back(
                        first.start + first.direction.normalized() * (edge.s0() - first.s0),
                        Vector2f(first.direction) * reverse_factor);
        }

        for (unsigned int i = edge.first_non_cut_segment() + 1; i <= edge.last_non_cut_segment(); ++i)
        {
            const sim::PhysicalEdgeSegment &segment = edge.segments()[i];
            line.emplace_back(
                        segment.start,
                        Vector2f(segment.direction) * reverse_factor
                        );
        }

        {
            const sim::PhysicalEdgeSegment &last = edge.segments()[edge.last_non_cut_segment()];
            line.emplace_back(
                        last.start + last.direction.normalized() * (edge.s1() - last.s0),
                        Vector2f(last.direction) * reverse_factor);
        }

        if (line.size() > 1) {
            segment_count += line.size() - 1;
            vertex_count += line.size();
        } else {
            line.clear();
        }

    }

    if (m_ibo_alloc) {
        m_ibo_alloc = nullptr;
        m_vbo_alloc = nullptr;
    }

    m_ibo_alloc = m_material.ibo().allocate(segment_count*2);
    m_vbo_alloc = m_material.vbo().allocate(vertex_count);

    uint16_t *dest = m_ibo_alloc.get();
    unsigned int vertex_counter = 0;
    auto positions = VBOSlice<Vector3f>(m_vbo_alloc, 0);
    auto directions = VBOSlice<Vector2f>(m_vbo_alloc, 1);
    for (const std::vector<std::tuple<Vector3f, Vector2f> > &line: lines)
    {
        if (line.empty()) {
            continue;
        }

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

    if (!octree()) {
        m_octree.insert_object(this);
    }
}

}
