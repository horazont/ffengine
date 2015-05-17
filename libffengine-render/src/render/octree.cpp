/**********************************************************************
File name: octree.cpp
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
#include "ffengine/render/octree.hpp"

#include <cassert>


namespace engine {

/* engine::OctreeNode */

const unsigned int OctreeNode::SPLIT_THRESHOLD = 8*2;
const unsigned int OctreeNode::STRADDLE_THRESHOLD_DIVISOR = 4;

OctreeNode &OctreeNode::autocreate_child(unsigned int i)
{
    assert(i < 8);
    if (!m_children[i]) {
        m_children[i] = std::make_unique<OctreeNode>(m_root, this);
    }
    return *m_children[i];
}

unsigned int OctreeNode::find_child_for(const OctreeObject *obj)
{
    unsigned int destination = 0;
    for (unsigned int i = 0; i < 3; ++i)
    {
        if (!m_split_planes[i].enabled) {
            destination <<= 1;
            continue;
        }

        PlaneSide side = m_split_planes[i].plane.side_of(obj->m_bounding_sphere);
        if (side == PlaneSide::POSITIVE_NORMAL) {
            destination |= 1;
        } else if (side == PlaneSide::BOTH) {
            return CHILD_SELF;
        }
        destination <<= 1;
    }

    return destination;
}

bool OctreeNode::merge()
{
    assert(m_is_split);

    for (unsigned int i = 0; i < 8; ++i) {
        if (m_children[i] && !m_children[i]->m_is_split) {
            // check whether any child node is split, we cannot merge then
            return false;
        }
    }

    for (unsigned int i = 0; i < 8; ++i) {
        OctreeNode *child = m_children[i].get();
        if (!child) {
            continue;
        }

        std::move(child->m_objects.begin(),
                  child->m_objects.end(),
                  std::back_inserter(m_objects));

        m_children[i] = nullptr;
    }

    m_split_planes[0].enabled = false;
    m_split_planes[1].enabled = false;
    m_split_planes[2].enabled = false;

    m_is_split = false;
    return true;
}

bool OctreeNode::split()
{
    assert(!m_is_split);

    if (m_objects.size() == 0) {
        return false;
    }

    Vector3f mean(0, 0, 0);
    float mean_sum = 0.f;
    for (auto &child: m_objects) {
        float weight = 1.f / child->m_bounding_sphere.radius;
        mean_sum += weight;
        mean += child->m_bounding_sphere.center * weight;
    }

    mean /= mean_sum;

    m_split_planes[0].plane = Plane(mean, Vector3f(1, 0, 0));
    m_split_planes[0].enabled = true;
    m_split_planes[1].plane = Plane(mean, Vector3f(0, 1, 0));
    m_split_planes[1].enabled = true;
    m_split_planes[2].plane = Plane(mean, Vector3f(0, 0, 1));
    m_split_planes[2].enabled = true;

    std::vector<unsigned int> object_child_indices;
    std::array<unsigned int, 3> straddle_counters({0, 0, 0});
    object_child_indices.reserve(m_objects.size());
    for (unsigned int i = 0;
         i < m_objects.size();
         ++i)
    {
        for (unsigned int plane_idx = 0; plane_idx < 3; ++plane_idx)
        {
            PlaneSide side = m_split_planes[plane_idx].plane.side_of(
                        m_objects[i]->m_bounding_sphere);;
            if (side == PlaneSide::BOTH) {
                straddle_counters[plane_idx] += 1;
            }
        }
    }

    // see how the straddles distribute among the planes, disable planes with
    // too much straddling
    const unsigned int straddle_threshold = (m_objects.size() + STRADDLE_THRESHOLD_DIVISOR - 1) / STRADDLE_THRESHOLD_DIVISOR;
    unsigned int disabled_count = 0;
    unsigned int max_straddling_plane = 3;
    unsigned int max_straddling = 0;
    for (unsigned int i = 0; i < 3; ++i)
    {
        if (straddle_counters[i] > max_straddling) {
            max_straddling = straddle_counters[i];
            max_straddling_plane = i;
        }
        if (straddle_counters[i] > straddle_threshold) {
            m_split_planes[i].enabled = false;
            disabled_count++;
        }
    }

    if (disabled_count == 3) {
        // re-enable all planes but the one with most straddling
        for (unsigned int i = 0; i < 3; ++i) {
            if (i != max_straddling_plane) {
                m_split_planes[i].enabled = true;
            }
        }
    }

    // now, sort all objects into children
    for (auto iter = m_objects.begin();
         iter != m_objects.end();
         ++iter)
    {
        OctreeObject *obj = *iter;
        unsigned int destination;
        destination = find_child_for(*iter);
        if (destination != CHILD_SELF) {
            iter = m_objects.erase(iter);
            autocreate_child(destination).insert_object(obj);
        }
    }

    m_is_split = true;
    return true;
}

OctreeNode *OctreeNode::insert_object(OctreeObject *obj)
{
    unsigned int destination = find_child_for(obj);
    if (destination == CHILD_SELF) {
        m_objects.push_back(obj);
        obj->m_parent = this;
    } else {
        return autocreate_child(destination).insert_object(obj);
    }
    return this;
}

/* engine::Octree */

Octree::Octree():
    m_root(*this)
{

}

}
