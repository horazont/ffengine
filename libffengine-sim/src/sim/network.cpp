/**********************************************************************
File name: network.cpp
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
#include "ffengine/sim/network.hpp"

#include "ffengine/math/curve.hpp"


namespace sim {

const EdgeClass EDGE_CLASS_ROAD;
const EdgeType EDGE_TYPE_BIDIRECTIONAL_ONE_LANE(
        EDGE_CLASS_ROAD,
        1, 0, 1, true);
const EdgeType EDGE_TYPE_BIDIRECTIONAL_THREE_LANES(
        EDGE_CLASS_ROAD,
        3, 0.5, 1, true);


/* sim::EdgeType */

EdgeType::EdgeType(const EdgeClass &class_):
    m_class(class_),
    m_lanes(1),
    m_lane_padding(0),
    m_lane_center_margin(0),
    m_bidirectional(false)
{

}

EdgeType::EdgeType(const EdgeClass &class_,
                   const unsigned int lanes,
                   const float lane_padding,
                   const float lane_center_margin,
                   const bool bidirectional):
    m_class(class_),
    m_lanes(lanes),
    m_lane_padding(lane_padding),
    m_lane_center_margin(lane_center_margin),
    m_bidirectional(bidirectional)
{

}


/* sim::PhysicalEdgeSegment */

PhysicalEdgeSegment::PhysicalEdgeSegment(const float s0,
                                         const Vector3f &start,
                                         const Vector3f &direction):
    s0(s0),
    start(start),
    direction(direction)
{

}

bool PhysicalEdgeSegment::operator==(const PhysicalEdgeSegment &other) const
{
    return (s0 == other.s0 &&
            start == other.start &&
            direction == other.direction);
}


/* sim::PhysicalEdge */

PhysicalEdge::PhysicalEdge(PhysicalEdgeBundle &parent,
                           bool reversed,
                           std::vector<PhysicalEdgeSegment> &&segments):
    m_parent(parent),
    m_reversed(reversed),
    m_segments(std::move(segments)),
    m_s0(0),
    m_s1(m_segments.back().s0 + m_segments.back().direction.length())
{

}


/* sim::PhysicalEdgeBundle */

PhysicalEdgeBundle::PhysicalEdgeBundle(
        const Object::ID object_id,
        const EdgeType &type,
        const object_ptr<PhysicalNode> &start_node,
        const object_ptr<PhysicalNode> &end_node):
    Object(object_id),
    m_type(type),
    m_start_node(start_node),
    m_end_node(end_node),
    m_flat(true),
    m_control_point(NAN, NAN, NAN),
    m_segments({PhysicalEdgeSegment(
                   0,
                   m_start_node->position(),
                   m_end_node->position() - m_start_node->position())})
{
    apply_type(m_type);
}

PhysicalEdgeBundle::PhysicalEdgeBundle(
        const Object::ID object_id,
        const EdgeType &type,
        const object_ptr<PhysicalNode> &start_node,
        const object_ptr<PhysicalNode> &end_node,
        const Vector3f &control_point):
    Object(object_id),
    m_type(type),
    m_start_node(start_node),
    m_end_node(end_node),
    m_flat(false),
    m_control_point(control_point)
{
    const Vector3f start_point(m_start_node->position());
    const Vector3f end_point(m_end_node->position());

    QuadBezier3f curve(start_point,
                       m_control_point,
                       end_point);
    std::vector<float> ts;
    autosample_quadbezier(curve, std::back_inserter(ts));
    // erase 0
    ts.erase(ts.begin());

    float s = 0;
    Vector3f prev_point = start_point;
    for (float t: ts)
    {
        const Vector3f curr_point(curve[t]);
        const Vector3f direction(curr_point - prev_point);
        m_segments.emplace_back(s, prev_point, direction);
        s += direction.length();
    }

    apply_type(m_type);
}

void PhysicalEdgeBundle::add_edge(const float offset,
                                  const EdgeDirection direction)
{
    float adapted_offset = (direction == EdgeDirection::FORWARD
                            ? offset
                            : -offset);

    std::vector<PhysicalEdgeSegment> segments;
    offset_segments(m_segments, adapted_offset,
                    (m_control_point - m_start_node->position()).normalized(),
                    (m_end_node->position() - m_control_point).normalized(),
                    segments);
    if (direction == EdgeDirection::REVERSE) {
        m_edges.emplace_back(std::make_unique<PhysicalEdge>(
                                 *this, true,
                                 std::move(segments)
                                 ));
    } else {
        m_edges.emplace_back(std::make_unique<PhysicalEdge>(
                                 *this, false,
                                 std::move(segments)
                                 ));
    }
}

void PhysicalEdgeBundle::apply_type(const EdgeType &type)
{
    m_edges.clear();
    if (type.m_bidirectional) {
        float offset = type.m_lane_center_margin / 2.f;
        for (unsigned int lane = 0; lane < type.m_lanes; ++lane)
        {
            add_edge(offset, EdgeDirection::FORWARD);
            add_edge(offset, EdgeDirection::REVERSE);
            offset += type.m_lane_padding;
        }
    } else {
        if ((type.m_lanes & 1) == 1) {
            // odd number of lanes, this leads to center lane
            add_edge(0, EdgeDirection::FORWARD);
        }
        float offset = type.m_lane_center_margin / 2.f;
        for (unsigned int lane = 0; lane < type.m_lanes / 2; ++lane)
        {
            add_edge(offset, EdgeDirection::FORWARD);
            add_edge(-offset, EdgeDirection::FORWARD);
            offset += type.m_lane_padding;
        }
    }
}


/* PhysicalNode */

PhysicalNode::PhysicalNode(const Object::ID object_id,
                           const EdgeClass &class_,
                           const Vector3f &position):
    Object::Object(object_id),
    m_class(class_),
    m_position(position)
{

}

/* PhysicalGraph */

PhysicalGraph::PhysicalGraph(ObjectManager &objects):
    m_objects(objects)
{

}

PhysicalEdgeBundle &PhysicalGraph::create_bundle(
        PhysicalNode &start_node,
        const Vector3f &control_point,
        PhysicalNode &end_node,
        const EdgeType &type)
{
    PhysicalEdgeBundle &bundle = m_objects.allocate<PhysicalEdgeBundle>(
                type,
                m_objects.share(start_node),
                m_objects.share(end_node),
                control_point);
    m_edge_bundle_created(m_objects.share(bundle));
    return bundle;
}

void PhysicalGraph::construct_curve(
        PhysicalNode &start_node,
        const Vector3f &control_point,
        PhysicalNode &end_node,
        const EdgeType &type)
{
    std::vector<QuadBezier3f> segments;
    segmentize_curve(QuadBezier3f(start_node.position(),
                                  control_point,
                                  end_node.position()),
                     segments);
    assert(segments.size() > 0);

    if (segments.size() == 1) {
        create_bundle(start_node, control_point, end_node, type);
        return;
    }

    PhysicalNode *prev_node = &start_node;
    for (unsigned int i = 0; i < segments.size()-1; ++i)
    {
        PhysicalNode &new_node = create_node(type.m_class, segments[i].p_end);
        create_bundle(*prev_node, segments[i].p_control, new_node,
                      type);
        prev_node = &new_node;
    }

    create_bundle(*prev_node, segments.back().p_control, end_node, type);
}

PhysicalNode &PhysicalGraph::create_node(const EdgeClass &class_,
                                         const Vector3f &position)
{
    PhysicalNode &node = m_objects.allocate<PhysicalNode>(class_, position);
    m_node_created(m_objects.share(node));
    return node;
}


void offset_segments(const std::vector<PhysicalEdgeSegment> &segments,
                     const float offset,
                     const Vector3f &entry_direction,
                     const Vector3f &exit_direction,
                     std::vector<PhysicalEdgeSegment> &dest)
{
    if (segments.empty()) {
        return;
    }

    const Vector3f up(0, 0, 1);
    float s = segments[0].s0;
    Vector3f prev_end(segments[0].start);
    {
        Vector3f bitangent(segments[0].direction.normalized() % up);
        bitangent.normalize();

        Vector3f entry_bitangent((entry_direction % up).normalized());
        entry_bitangent += bitangent;
        entry_bitangent.normalize();

        entry_bitangent = entry_bitangent / (entry_bitangent*bitangent) * offset;

        prev_end += entry_bitangent;
    }

    for (unsigned int i = 0; i < segments.size(); ++i)
    {
        const PhysicalEdgeSegment &segment = segments[i];

        const Vector3f start(prev_end);
        const Vector3f bitangent((segment.direction.normalized() % up).normalized());

        Vector3f curr_bitangent(bitangent);
        if (i == segments.size() - 1) {
            curr_bitangent += (exit_direction % up).normalized();
        } else {
            curr_bitangent += (segments[i+1].direction.normalized() % up).normalized();
        }
        curr_bitangent.normalize();

        curr_bitangent = curr_bitangent / (curr_bitangent * bitangent) * offset;

        const Vector3f end(segment.start + segment.direction + curr_bitangent);

        prev_end = end;

        const Vector3f direction(end - start);
        dest.emplace_back(s, start, direction);
        s += direction.length();
    }
}


std::ostream &operator<<(std::ostream &dest, const PhysicalEdgeSegment &segment)
{
    return dest << "PhysicalEdgeSegment("
         << segment.s0
         << ", " << segment.start
         << ", " << segment.direction
         << ")";
}


void segmentize_curve(const QuadBezier3f &curve,
                      std::vector<QuadBezier3f> &segments)
{
    const float segment_length = 10;
    const float min_length = 5;
    std::vector<float> ts;
    // we use the autosampled points as a reference for where we can approximate
    // the curve using line segments. inside those segments, we split as
    // neccessary
    autosample_quadbezier(curve, std::back_inserter(ts));

    std::vector<float> segment_ts;

    float len_accum = 0.f;
    float prev_t = 0.f;
    Vector3f prev_point = curve[0.];
    for (float sampled_t: ts) {
        Vector3f curr_point = curve[sampled_t];
        float segment_len = (prev_point - curr_point).length();
        float existing_len = len_accum;
        float split_t = 0.f;
        len_accum += segment_len;

        if (len_accum >= segment_length) {
            // special handling for re-using the existing length
            float local_len = segment_length - existing_len;
            split_t = prev_t + (sampled_t - prev_t) * local_len / segment_len;
            segment_ts.push_back(split_t);

            len_accum -= segment_length;
        }

        while (len_accum >= segment_length) {
            split_t += (sampled_t - prev_t) * segment_length / segment_len;
            len_accum -= segment_length;
            segment_ts.push_back(split_t);
        }

        prev_t = sampled_t;
        prev_point = curr_point;
    }

    // drop the last segment if it would otherwise result in a very short piece
    if (len_accum < min_length) {
        segment_ts.pop_back();
    }

    curve.segmentize(segment_ts.begin(), segment_ts.end(),
                     std::back_inserter(segments));
}


}
