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

#include <algorithm>
#include <iostream>

#include "ffengine/math/curve.hpp"
#include "ffengine/math/line.hpp"

#include "ffengine/math/tikz.hpp"


namespace sim {

const EdgeClass EDGE_CLASS_ROAD;
const EdgeType EDGE_TYPE_BIDIRECTIONAL_ONE_LANE(
        EDGE_CLASS_ROAD,
        1, 0, 1, true,
        0.5f);
const EdgeType EDGE_TYPE_BIDIRECTIONAL_THREE_LANES(
        EDGE_CLASS_ROAD,
        3, 0.5, 1, true,
        1.5f);


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
                   const bool bidirectional,
                   const float half_cut_width):
    m_class(class_),
    m_lanes(lanes),
    m_lane_padding(lane_padding),
    m_lane_center_margin(lane_center_margin),
    m_bidirectional(bidirectional),
    m_half_cut_width(half_cut_width)
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
    m_len(m_segments.back().s0 + m_segments.back().direction.length()),
    m_s0(0),
    m_s1(m_len)
{

}

void PhysicalEdge::cut_s0(const Line2f &cut_line)
{
    std::cout << std::endl << std::endl;
    std::cout << "\\begin{scope}[xshift=" << (-m_segments[0].start[eX]) << ",yshift=" << (-m_segments[0].start[eY]) << "]" << std::endl;
    std::cout << "% member of bundle " << &m_parent << std::endl;
    std::cout << "% cutting s0 at " << cut_line << std::endl;
    tikz_draw_line_around_origin(
                std::cout,
                isect_line_line(cut_line, Line2f(Vector2f(m_segments[0].start), Vector2f(m_segments[0].direction))),
                cut_line.point_and_direction().second.normalized() * 2.f,
                0.5f,
                "help lines");
    for (unsigned int i = 0; i < m_segments.size(); ++i)
    {
        const PhysicalEdgeSegment &segment = m_segments[i];
        const Vector2f start(segment.start);
        const Vector2f direction(segment.direction);
        const Vector2f direction_normalized(direction.normalized());

        tikz_draw(std::cout,
                  start, direction, "-latex");

        Line2f segment_line(start, direction_normalized);
        Vector2f intersection = isect_line_line(cut_line, segment_line);
        tikz_node(std::cout, intersection, "", "inner sep=0.1mm,draw,rectangle,gray");
        if (std::isnan(intersection[eX])) {
            continue;
        }

        const float intersection_along_direction = (intersection-start) * direction_normalized;
        std::cout << "% intersection_along_direction " << intersection_along_direction << std::endl;
        if (intersection_along_direction >= direction.length()) {
            // intersection point is behind end of segment, continue
            continue;
        }

        m_s0 = segment.s0 + intersection_along_direction;
        m_first_non_cut_segment = i;
        std::cout << "% cut point found at segment " << i << " with s0 = " << m_s0 << std::endl;
        break;
    }
    std::cout << "\\end{scope}" << std::endl;
    std::cout << std::endl << std::endl;
}

void PhysicalEdge::cut_s1(const Line2f &cut_line)
{
    std::cout << std::endl << std::endl;
    std::cout << "\\begin{scope}[xshift=" << (-m_segments[0].start[eX]) << ",yshift=" << (-m_segments[0].start[eY]) << "]" << std::endl;
    std::cout << "% member of bundle " << &m_parent << std::endl;
    std::cout << "% cutting s1 at " << cut_line << std::endl;
    tikz_draw_line_around_origin(
                std::cout,
                isect_line_line(cut_line, Line2f(Vector2f(m_segments[0].start), Vector2f(m_segments[0].direction))),
                cut_line.point_and_direction().second.normalized() * 2.f,
                0.5f,
                "help lines");

    unsigned int i = m_segments.size() - 1;
    do {
        const PhysicalEdgeSegment &segment = m_segments[i];
        const Vector2f start(segment.start);
        const Vector2f direction(segment.direction);
        const Vector2f direction_normalized(direction.normalized());

        tikz_draw(std::cout,
                  start, direction, "-latex");

        Line2f segment_line(start, direction_normalized);
        Vector2f intersection = isect_line_line(cut_line, segment_line);
        tikz_node(std::cout, intersection, "", "inner sep=0.1mm,draw,rectangle,gray");
        if (std::isnan(intersection[eX])) {
            --i;
            continue;
        }

        const float intersection_along_direction = (intersection-start) * direction_normalized;
        std::cout << "% intersection_along_direction " << intersection_along_direction << std::endl;
        if (intersection_along_direction < 0.f) {
            // intersection point is behind end of segment, continue
            --i;
            continue;
        }

        m_s1 = segment.s0 + intersection_along_direction;
        m_last_non_cut_segment = i;
        std::cout << "% cut point found at segment " << i << " with s1 = " << m_s1 << " / " << m_len << std::endl;
        break;
    } while (i > 0);

    std::cout << "\\end{scope}" << std::endl;
    std::cout << std::endl << std::endl;
}

void PhysicalEdge::set_s0(const float new_s0)
{
    m_s0 = new_s0;
}

void PhysicalEdge::set_s1(const float new_s1)
{
    m_s1 = new_s1;
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
    m_reshape_pending(true),
    m_segments({PhysicalEdgeSegment(
                   0,
                   m_start_node->position(),
                   m_end_node->position() - m_start_node->position())})
{
    apply_type();
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
    m_control_point(control_point),
    m_reshape_pending(true)
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

    apply_type();
}

void PhysicalEdgeBundle::add_edge(const float offset,
                                  const EdgeDirection direction)
{
    float adapted_offset = (direction == EdgeDirection::FORWARD
                            ? offset
                            : -offset);

    std::vector<PhysicalEdgeSegment> segments;
    offset_segments(m_segments, adapted_offset,
                    start_tangent().normalized(),
                    end_tangent().normalized(),
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

void PhysicalEdgeBundle::apply_type()
{

    m_edges.clear();
    if (m_type.m_bidirectional) {
        float offset = m_type.m_lane_center_margin / 2.f;
        for (unsigned int lane = 0; lane < m_type.m_lanes; ++lane)
        {
            add_edge(offset, EdgeDirection::FORWARD);
            add_edge(offset, EdgeDirection::REVERSE);
            offset += m_type.m_lane_padding;
        }
    } else {
        if ((m_type.m_lanes & 1) == 1) {
            // odd number of lanes, this leads to center lane
            add_edge(0, EdgeDirection::FORWARD);
        }
        float offset = m_type.m_lane_center_margin / 2.f;
        for (unsigned int lane = 0; lane < m_type.m_lanes / 2; ++lane)
        {
            add_edge(offset, EdgeDirection::FORWARD);
            add_edge(-offset, EdgeDirection::FORWARD);
            offset += m_type.m_lane_padding;
        }
    }
}

Vector3f PhysicalEdgeBundle::end_tangent() const
{
    if (m_flat) {
        return m_start_node->position() - m_end_node->position();
    } else {
        return m_end_node->position() - m_control_point;
    }
}

void PhysicalEdgeBundle::mark_for_reshape()
{
    m_reshape_pending = true;
}

bool PhysicalEdgeBundle::reshape()
{
    if (!m_reshape_pending) {
        return false;
    }

    const Line2f cut_start = m_start_node->bundle_cut(*this);
    const Line2f cut_end = m_end_node->bundle_cut(*this);

    for (std::unique_ptr<PhysicalEdge> &edge: m_edges) {
        edge->cut_s0(cut_start);
        edge->cut_s1(cut_end);
    }

    m_reshape_pending = false;
    return true;
}

Vector3f PhysicalEdgeBundle::start_tangent() const
{
    if (m_flat) {
        return m_end_node->position() - m_start_node->position();
    } else {
        return m_control_point - m_start_node->position();
    }
}


/* PhysicalNode::ExitRecord */

PhysicalNode::ExitRecord::ExitRecord(
        const object_ptr<PhysicalEdgeBundle> &bundle,
        bool start_is_here):
    m_bundle(bundle),
    m_start_is_here(start_is_here),
    m_exit_vector(get_naive_exit_vector()),
    m_exit_angle(std::atan2(m_exit_vector[eY], m_exit_vector[eX])),
    m_cut_line(Vector2f(0, 0), Vector2f(0, 0))
{

}

Vector3f PhysicalNode::ExitRecord::get_naive_exit_vector() const
{
    if (m_start_is_here) {
        return m_bundle->start_tangent();
    } else {
        return -m_bundle->end_tangent();
    }
}


/* PhysicalNode */

PhysicalNode::PhysicalNode(const Object::ID object_id,
                           const EdgeClass &class_,
                           const Vector3f &position):
    Object::Object(object_id),
    m_class(class_),
    m_reshape_pending(false),
    m_position(position)
{

}

const PhysicalNode::ExitRecord *PhysicalNode::record_for_bundle(
        const PhysicalEdgeBundle &bundle) const
{
    auto iter = std::find_if(m_exits.begin(), m_exits.end(),
                             [this, &bundle](const ExitRecord &rec){ return rec.m_bundle.get() == &bundle; });
    if (iter == m_exits.end()) {
        return nullptr;
    }
    return &(*iter);
}

void PhysicalNode::mark_for_reshape()
{
    m_reshape_pending = true;
}

void PhysicalNode::reshape()
{
    if (!m_reshape_pending) {
        return;
    }
    m_reshape_pending = false;

    if (m_exits.empty()) {
        return;
    }

    const Vector2f position_2f(m_position);
    const Vector3f up(0, 0, 1);

    std::cout << "\\begin{scope}[xshift=" << (-m_position[eX]) << ", yshift=" << (-m_position[eY]) << ", xscale=0.5, yscale=0.5]" << std::endl;

    if (m_exits.size() == 1) {
        // single exit
        m_exits[0].m_base_cut = 0.f;
        m_exits[0].m_exit_vector = m_exits[0].get_naive_exit_vector();
        std::cout << "% only one exit" << std::endl;
        std::cout << "% bundle " << m_exits[0].m_bundle.get() << std::endl;
    } else {
        std::sort(m_exits.begin(), m_exits.end(),
                  [](const ExitRecord &a, const ExitRecord &b){ return a.m_exit_angle < b.m_exit_angle; });

        for (auto &exit: m_exits) {
            exit.m_base_cut = 0;
        }

        for (unsigned int i = 0; i < m_exits.size(); ++i) {
            ExitRecord &first = m_exits[i];
            std::cout << "% bundle " << first.m_bundle.get() << std::endl;
            ExitRecord &second = m_exits[(i+1)%m_exits.size()];

            const Vector3f first_exit_vector_3f(first.get_naive_exit_vector().normalized());
            const Vector3f second_exit_vector_3f(second.get_naive_exit_vector().normalized());

            const Vector2f first_exit_vector(first_exit_vector_3f);
            const Vector2f second_exit_vector(second_exit_vector_3f);

            const Vector2f first_normal(Vector2f(first_exit_vector_3f % up).normalized());
            const Vector2f second_normal(Vector2f(second_exit_vector_3f % up).normalized());

            const Line2f first_right(
                        position_2f + first_normal * first.m_bundle->type().m_half_cut_width,
                        first_exit_vector);
            const Line2f first_left(
                        position_2f - first_normal * first.m_bundle->type().m_half_cut_width,
                        first_exit_vector);

            const Line2f second_right(
                        position_2f + second_normal * second.m_bundle->type().m_half_cut_width,
                        second_exit_vector);
            const Line2f second_left(
                        position_2f - second_normal * second.m_bundle->type().m_half_cut_width,
                        second_exit_vector);

            const Vector2f intersection_1 = isect_line_line(first_right, second_left);
            const Vector2f intersection_2 = isect_line_line(second_right, first_left);

            float first_cut = std::max((intersection_1 - position_2f) * first_exit_vector,
                                       (intersection_2 - position_2f) * first_exit_vector);
            float second_cut = std::max((intersection_1 - position_2f) * second_exit_vector,
                                        (intersection_2 - position_2f) * second_exit_vector);

            tikz_draw(std::cout,
                      Vector2f(m_position),
                      Vector2f(first_exit_vector * 10.f),
                      "->",
                      "$t_{" + std::to_string(i) + "}$");
            tikz_draw(std::cout,
                      Vector2f(m_position),
                      Vector2f(first_normal * first.m_bundle->type().m_half_cut_width),
                      "help lines");
            tikz_draw_line_around_origin(std::cout,
                                         position_2f + first_normal * first.m_bundle->type().m_half_cut_width,
                                         first_exit_vector * 10.f,
                                         0.1f,
                                         "red");
            tikz_draw_line_around_origin(std::cout,
                                         position_2f - first_normal * first.m_bundle->type().m_half_cut_width,
                                         first_exit_vector * 10.f,
                                         0.1f,
                                         "blue");
            tikz_node(std::cout, intersection_1, "", "draw,rectangle,inner sep=1mm");
            tikz_node(std::cout, intersection_2, "", "draw,rectangle,inner sep=1mm");

            std::cout << "% " << first_cut << " " << second_cut << std::endl;

            if (!std::isnan(first_cut)) {
                first.m_base_cut = std::max(first.m_base_cut, first_cut);
            }
            if (!std::isnan(second_cut)) {
                second.m_base_cut = std::max(second.m_base_cut, second_cut);
            }
        }
    }

    for (auto &exit: m_exits) {
        const Vector3f exit_vector_3f(exit.get_naive_exit_vector().normalized());
        const Vector2f exit_vector(exit_vector_3f);
        const Vector2f normal(Vector2f(exit_vector_3f % up).normalized());
        if (std::fabs(exit.m_base_cut) < 10) {
            tikz_draw_line_around_origin(std::cout,
                                         position_2f + exit_vector * exit.m_base_cut,
                                         normal*exit.m_bundle->type().m_half_cut_width*2.5f,
                                         0.5f,
                                         "help lines,dashed");
        }

        exit.m_cut_line = Line2f(position_2f + exit_vector * exit.m_base_cut, normal);
        exit.m_bundle->mark_for_reshape();
    }

    std::cout << "\\end{scope}" << std::endl;
}

void PhysicalNode::register_edge_bundle(
        const object_ptr<PhysicalEdgeBundle> &edge)
{
    assert(edge->start_node().get() == this || edge->end_node().get() == this);
    m_exits.emplace_back(edge, edge->start_node().get() == this);
    m_reshape_pending = true;
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
    object_ptr<PhysicalEdgeBundle> bundle_ptr(m_objects.share(bundle));
    start_node.register_edge_bundle(bundle_ptr);
    end_node.register_edge_bundle(bundle_ptr);
    m_bundles.emplace_back(bundle_ptr);
    m_edge_bundle_created(bundle_ptr);
    return bundle;
}

void PhysicalGraph::construct_curve(
        PhysicalNode &start_node,
        const Vector3f &control_point,
        PhysicalNode &end_node,
        const EdgeType &type)
{
    std::vector<PhysicalNode*> affected_nodes;
    std::vector<QuadBezier3f> segments;
    segmentize_curve(QuadBezier3f(start_node.position(),
                                  control_point,
                                  end_node.position()),
                     segments);
    assert(segments.size() > 0);

    if (segments.size() == 1) {
        affected_nodes.emplace_back(&start_node);
        affected_nodes.emplace_back(&end_node);
        create_bundle(start_node, control_point, end_node, type);
    } else {
        PhysicalNode *prev_node = &start_node;
        affected_nodes.emplace_back(prev_node);
        for (unsigned int i = 0; i < segments.size()-1; ++i)
        {
            PhysicalNode &new_node = create_node(type.m_class, segments[i].p_end);
            create_bundle(*prev_node, segments[i].p_control, new_node,
                          type);
            prev_node = &new_node;
            affected_nodes.emplace_back(prev_node);
        }

        create_bundle(*prev_node, segments.back().p_control, end_node, type);
        affected_nodes.emplace_back(&end_node);
    }

    for (PhysicalNode *node: affected_nodes) {
        node->mark_for_reshape();
    }
}

PhysicalNode &PhysicalGraph::create_node(const EdgeClass &class_,
                                         const Vector3f &position)
{
    PhysicalNode &node = m_objects.allocate<PhysicalNode>(class_, position);
    object_ptr<PhysicalNode> node_ptr(m_objects.share(node));
    m_nodes.emplace_back(node_ptr);
    m_node_created(node_ptr);
    return node;
}

void PhysicalGraph::reshape()
{
    {
        auto iter = m_nodes.begin();
        while (iter != m_nodes.end())
        {
            if (!(*iter)) {
                iter = m_nodes.erase(iter);
                continue;
            }
            (*iter)->reshape();
            ++iter;
        }
    }

    {
        auto iter = m_bundles.begin();
        while (iter != m_bundles.end())
        {
            if (!(*iter)) {
                iter = m_bundles.erase(iter);
                continue;
            }
            if ((*iter)->reshape()) {
                m_edge_bundle_reshaped(*iter);
            }
            ++iter;
        }
    }
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
