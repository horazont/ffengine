/**********************************************************************
File name: network.hpp
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
#ifndef SCC_SIM_NETWORK_H
#define SCC_SIM_NETWORK_H

// NOTE: This file has nothing to do with computer networks. It deals with the
// simulation of the transport network!

#include <ostream>

#include <sig11/sig11.hpp>

#include "ffengine/common/utils.hpp"

#include "ffengine/math/curve.hpp"
#include "ffengine/math/vector.hpp"

#include "ffengine/sim/objects.hpp"



namespace sim {


class PhysicalNode;
class PhysicalEdge;
class PhysicalEdgeBundle;


enum class EdgeDirection
{
    FORWARD = 0,
    REVERSE = 1
};


struct EdgeClass
{
    EdgeClass() = default;

    EdgeClass(const EdgeClass &ref) = delete;
    EdgeClass &operator=(const EdgeClass &ref) = delete;
    EdgeClass(EdgeClass &&src) = delete;
    EdgeClass &operator=(EdgeClass &&src) = delete;


    bool operator==(const EdgeClass &other) const
    {
        return this == &other;
    }

    bool operator!=(const EdgeClass &other) const
    {
        return this != &other;
    }

};


struct EdgeType
{
    EdgeType() = delete;

    EdgeType(const EdgeClass &class_);
    EdgeType(const EdgeClass &class_,
             const unsigned int lanes,
             const float lane_padding,
             const float lane_center_margin,
             const bool bidirectional);

    // EdgeType instances are unique
    EdgeType(const EdgeType &ref) = delete;
    EdgeType &operator=(const EdgeType &ref) = delete;
    EdgeType(EdgeType &&src) = delete;
    EdgeType &operator=(EdgeType &&src) = delete;


    const EdgeClass &m_class;
    unsigned int m_lanes;
    float m_lane_padding;
    float m_lane_center_margin;
    bool m_bidirectional;


    bool operator==(const EdgeType &other) const
    {
        return this == &other;
    }

    bool operator!=(const EdgeType &other) const
    {
        return this != &other;
    }

};


struct PhysicalEdgeSegment
{
    PhysicalEdgeSegment(const float s0,
                        const Vector3f &start,
                        const Vector3f &direction);

    float s0;
    Vector3f start;
    Vector3f direction;

    bool operator==(const PhysicalEdgeSegment &other) const;

    inline bool operator!=(const PhysicalEdgeSegment &other) const
    {
        return !((*this) == other);
    }

};


/**
 * A single edge in the physical graph. Note that edges are always owned by an
 * edge bundle and cannot be deleted individually, which is why they do not
 * inherit from Object.
 *
 * A physical edge does not consist of a curve anymore. Instead, it uses the
 * "flattened" curve, i.e. the curve split into linear parts. This helps
 * with offsetting the edge from the center.
 *
 * A physical edge is most likely "cut", that is, has non-zero s0() and non-one
 * s1(). In such a case, it actually starts and ends inside its own geometry,
 * the s-values give the length at which the edge actually starts. This is used
 * to cut an edge when a junction increases in size, for example due to a
 * larger road type interconnecting.
 */
class PhysicalEdge
{
public:
    PhysicalEdge(PhysicalEdgeBundle &parent,
                 bool reversed,
                 std::vector<PhysicalEdgeSegment> &&segments);

private:
    PhysicalEdgeBundle &m_parent;
    const bool m_reversed;
    const std::vector<PhysicalEdgeSegment> m_segments;

    float m_s0, m_s1;

    // TODO: type-specific information for the edge

public:
    inline bool reversed() const
    {
        return m_reversed;
    }

    inline float s0() const
    {
        return m_s0;
    }

    inline float s1() const
    {
        return m_s1;
    }

    const std::vector<PhysicalEdgeSegment> &segments() const
    {
        return m_segments;
    }

};


/**
 * Represent a bundle of edges. These are created by constructing multi-lane
 * and/or bidirectional paths.
 *
 * An edge bundle is the required information for the physical and logical
 * layer to properly group edges together.
 */
class PhysicalEdgeBundle: public Object
{
public:
    using EdgeContainer = std::vector<std::unique_ptr<PhysicalEdge> >;
    using value_type = PhysicalEdge&;
    using iterator = ffe::DereferencingIterator<EdgeContainer::iterator, PhysicalEdgeBundle>;
    using const_iterator = ffe::DereferencingIterator<EdgeContainer::const_iterator, PhysicalEdgeBundle, const PhysicalEdge>;


public:
    PhysicalEdgeBundle(const ID object_id,
                       const EdgeType &type,
                       const object_ptr<PhysicalNode> &start_node,
                       const object_ptr<PhysicalNode> &end_node);
    PhysicalEdgeBundle(const ID object_id,
                       const EdgeType &type,
                       const object_ptr<PhysicalNode> &start_node,
                       const object_ptr<PhysicalNode> &end_node,
                       const Vector3f &control_point);

private:
    const EdgeType &m_type;

    const object_ptr<PhysicalNode> m_start_node;
    const object_ptr<PhysicalNode> m_end_node;

    const bool m_flat;
    const Vector3f m_control_point;

    std::vector<PhysicalEdgeSegment> m_segments;

    std::vector<std::unique_ptr<PhysicalEdge> > m_edges;

private:
    /**
     * Add a physical edge which is offset to the right in the direction of
     * driving.
     *
     * @param offset The distance of the offset physical edge.
     * @param direction The direction of the edge.
     */
    void add_edge(const float offset,
                  const EdgeDirection direction);

    void apply_type(const EdgeType &type);

public:
    inline bool flat() const
    {
        return m_flat;
    }

    inline const Vector3f &control_point() const
    {
        return m_control_point;
    }

    inline const object_ptr<PhysicalNode> &end_node() const
    {
        return m_end_node;
    }

    inline const object_ptr<PhysicalNode> &start_node() const
    {
        return m_start_node;
    }

    inline const_iterator begin() const
    {
        return const_iterator(m_edges.begin());
    }

    inline const_iterator end() const
    {
        return const_iterator(m_edges.end());
    }

};


class PhysicalNode: public Object
{
public:
    PhysicalNode(const ID object_id,
                 const EdgeClass &class_,
                 const Vector3f &position);

private:
    const EdgeClass &m_class;
    Vector3f m_position;
    std::vector<object_ptr<PhysicalEdgeBundle> > m_exits;

public:
    inline const Vector3f &position() const
    {
        return m_position;
    }

};


class PhysicalGraph
{
public:
    using EdgeBundleSignal = sig11::signal<void(object_ptr<PhysicalEdgeBundle>)>;
    using NodeSignal = sig11::signal<void(object_ptr<PhysicalNode>)>;

public:
    explicit PhysicalGraph(ObjectManager &objects);

private:
    ObjectManager &m_objects;

    mutable EdgeBundleSignal m_edge_bundle_created;
    mutable NodeSignal m_node_created;

private:
    PhysicalEdgeBundle &create_bundle(PhysicalNode &start_node,
                                      const Vector3f &control_point,
                                      PhysicalNode &end_node,
                                      const EdgeType &type);

public:
    void construct_curve(PhysicalNode &start_node,
                         const Vector3f &control_point,
                         PhysicalNode &end_node,
                         const EdgeType &type);
    PhysicalNode &create_node(const EdgeClass &class_,
                              const Vector3f &position);

public:
    inline EdgeBundleSignal &edge_bundle_created() const
    {
        return m_edge_bundle_created;
    }

    inline NodeSignal &node_created() const
    {
        return m_node_created;
    }

};


void offset_segments(const std::vector<PhysicalEdgeSegment> &segments,
                     const float offset,
                     const Vector3f &entry_direction,
                     const Vector3f &exit_direction,
                     std::vector<PhysicalEdgeSegment> &dest);


std::ostream &operator<<(std::ostream &dest, const PhysicalEdgeSegment &segment);


void segmentize_curve(const QuadBezier3f &curve,
                      std::vector<QuadBezier3f> &segments);


extern const EdgeClass EDGE_CLASS_ROAD;

extern const EdgeType EDGE_TYPE_BIDIRECTIONAL_ONE_LANE;

}

#endif
