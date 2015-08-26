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
#include "ffengine/math/octree.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "ffengine/math/intersect.hpp"


namespace ffe {

/* ffe::OctreeObject */

OctreeObject::OctreeObject():
    m_parent(nullptr),
    m_bounding_sphere{Vector3f(0, 0, 0), 0.f}
{

}

OctreeObject::~OctreeObject()
{
    if (m_parent) {
        m_parent->tree().remove_object(this);
    }
}

void OctreeObject::update_bounds(Sphere new_bounds)
{
    Octree *tree = nullptr;
    if (m_parent) {
        tree = &m_parent->tree();
        tree->remove_object(this);
    }
    m_bounding_sphere = new_bounds;
    if (tree) {
        tree->insert_object(this);
    }
}

const Octree *OctreeObject::octree() const
{
    if (m_parent) {
        return &m_parent->tree();
    }
    return nullptr;
}

Octree *OctreeObject::octree()
{
    if (m_parent) {
        return &m_parent->tree();
    }
    return nullptr;
}

/* ffe::OctreeNode */

const unsigned int OctreeNode::SPLIT_THRESHOLD = 8*2;
const unsigned int OctreeNode::STRADDLE_THRESHOLD_DIVISOR = 4;

OctreeNode::OctreeNode(Octree &tree):
    m_tree(tree),
    m_parent(nullptr),
    m_index_at_parent(CHILD_SELF),
    m_bounds_valid(false),
    m_is_split(false),
    m_nonempty_children(0)
{

}

OctreeNode::OctreeNode(OctreeNode &parent, unsigned int index):
    m_tree(parent.tree()),
    m_parent(&parent),
    m_index_at_parent(index),
    m_bounds_valid(false),
    m_is_split(false),
    m_nonempty_children(0)
{

}

OctreeNode::~OctreeNode()
{
    for (auto &obj: m_objects)
    {
        // unlink objects in case they’re still linked
        obj->m_parent = nullptr;
    }
}

OctreeNode &OctreeNode::autocreate_child(unsigned int i)
{
    assert(i < 8);
    if (!m_children[i]) {
        m_children[i] = std::unique_ptr<OctreeNode>(new OctreeNode(*this, i));
        m_nonempty_children += 1;
    }
    return *m_children[i];
}

const AABB &OctreeNode::updated_bounds() const
{
    if (m_bounds_valid) {
        return m_bounds;
    }

    m_bounds = AABB::Empty;

    if (m_is_split) {
        for (auto &child: m_children)
        {
            if (child) {
                m_bounds.extend_to_cover(child->bounds());
            }
        }
    }

    for (const OctreeObject *object: m_objects)
    {
        const Sphere &bounds = object->m_bounding_sphere;
        m_bounds.extend_to_cover(
                    AABB{bounds.center - Vector3f(bounds.radius, bounds.radius, bounds.radius),
                         bounds.center + Vector3f(bounds.radius, bounds.radius, bounds.radius)});
    }

    m_bounds_valid = true;

    return m_bounds;
}

void OctreeNode::delete_if_empty()
{
    if (!m_parent) {
        return;
    }
    if (!m_objects.empty()) {
        return;
    }
    if (m_is_split && m_nonempty_children > 0) {
        return;
    }

    // this deletes!
    m_parent->notify_empty_child(m_index_at_parent);
}

unsigned int OctreeNode::find_child_for(const OctreeObject *obj)
{
    const Sphere &bounding_sphere = obj->m_bounding_sphere;

    unsigned int destination = 0;
    for (unsigned int i = 0; i < 3; ++i)
    {
        destination <<= 1;
        if (!m_split_planes[i].enabled) {
            continue;
        }

        PlaneSide side = m_split_planes[i].plane.side_of(bounding_sphere);

        if (side == PlaneSide::POSITIVE_NORMAL) {
            destination |= 1;
        } else if (side == PlaneSide::BOTH) {
            return CHILD_SELF;
        }
    }

    return destination;
}

bool OctreeNode::merge()
{
    if (!m_is_split) {
        return true;
    }

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

        m_objects.reserve(m_objects.size() + child->m_objects.size());
        for (OctreeObject *obj: child->m_objects)
        {
            obj->m_parent = this;
            m_objects.push_back(obj);
        }
        child->m_objects.clear();

        m_children[i] = nullptr;
    }

    m_split_planes[0].enabled = false;
    m_split_planes[1].enabled = false;
    m_split_planes[2].enabled = false;

    m_is_split = false;
    return true;
}

void OctreeNode::notify_empty_child(unsigned int index)
{
    assert(index < 8);
    assert(m_children[index]);
    assert(m_nonempty_children > 0);

    m_children[index] = nullptr;
    --m_nonempty_children;

    if (m_nonempty_children == 0) {
        merge();
        if (m_objects.size() >= SPLIT_THRESHOLD) {
            split();
        }
    }

    delete_if_empty();
}

void OctreeNode::remove_object(OctreeObject *obj)
{
    auto iter = std::find(m_objects.begin(),
                          m_objects.end(),
                          obj);
    if (iter == m_objects.end())
    {
        return;
    }
    assert(obj->m_parent == this);
    m_objects.erase(iter);

    obj->m_parent = nullptr;
    // TODO: only invalidate bounds if the object was flush against the edges
    m_bounds_valid = false;

    delete_if_empty();
}

void OctreeNode::select_nodes_by_ray(const Ray &r,
                                     std::vector<OctreeRayHitInfo> &hitset)
{
    float t0, t1;
    if (!isect_aabb_ray(bounds(), r, t0, t1)) {
        return;
    }

    if (!m_objects.empty()) {
        hitset.push_back(OctreeRayHitInfo{this, t0, t1});
    }

    for (std::unique_ptr<OctreeNode> &child: m_children)
    {
        if (child) {
            child->select_nodes_by_ray(r, hitset);
        }
    }
}

void OctreeNode::select_nodes_with_objects(std::vector<OctreeNode *> &hitset)
{
    if (!m_objects.empty()) {
        hitset.push_back(this);
    }

    for (auto &child: m_children) {
        if (child) {
            child->select_nodes_with_objects(hitset);
        }
    }
}

void OctreeNode::select_nodes_by_frustum(
        const std::array<Plane, 6> &frustum,
        std::vector<OctreeNode*> &hitset)
{
    PlaneSide result = isect_aabb_frustum(bounds(), frustum);
    if (result == PlaneSide::NEGATIVE_NORMAL) {
        // entirely outside frustum
        return;
    } else if (result == PlaneSide::POSITIVE_NORMAL) {
        // entirely inside frustum
        select_nodes_with_objects(hitset);
        return;
    }

    // intersects with frustum, add ourselves if we have objects and
    // recurse into children
    if (!m_objects.empty()) {
        hitset.push_back(this);
    }

    for (auto &child: m_children)
    {
        if (child) {
            child->select_nodes_by_frustum(frustum, hitset);
        }
    }
}

bool OctreeNode::split()
{
    if (m_is_split) {
        return true;
    }

    if (m_objects.size() == 0) {
        return false;
    }

    Vector3f mean(0, 0, 0);
    float mean_sum = 0.f;
    for (auto &child: m_objects) {
        if (child->m_bounding_sphere.radius >= std::numeric_limits<float>::epsilon())
        {
            float weight = 1.f / child->m_bounding_sphere.radius;
            mean_sum += weight;
            mean += child->m_bounding_sphere.center * weight;
        }
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

    if (disabled_count >= 2) {
        // re-enable all planes but the one with most straddling
        for (unsigned int i = 0; i < 3; ++i) {
            m_split_planes[i].enabled = (i != max_straddling_plane);
        }
    }

    /*for (unsigned int i = 0; i < 3; ++i) {
        std::cout << "split plane " << i << ": "
                  << m_split_planes[i].plane
                  << " (enabled: " << m_split_planes[i].enabled << ")"
                  << std::endl;
    }*/

    // now, sort all objects into children
    auto iter = m_objects.begin();
    while (iter != m_objects.end())
    {
        OctreeObject *obj = *iter;
        unsigned int destination;
        destination = find_child_for(*iter);
        if (destination != CHILD_SELF) {
            iter = m_objects.erase(iter);
            autocreate_child(destination).insert_object(obj);
        } else {
            ++iter;
        }
    }

    m_is_split = true;
    return true;
}

OctreeNode *OctreeNode::insert_object(OctreeObject *obj)
{
    unsigned int destination;
    if (m_is_split) {
        destination = find_child_for(obj);
    } else {
        destination = CHILD_SELF;
    }

    if (destination == CHILD_SELF) {
        m_objects.push_back(obj);
        obj->m_parent = this;
        if (!m_is_split && m_objects.size() >= SPLIT_THRESHOLD) {
            split();
        }
    } else {
        return autocreate_child(destination).insert_object(obj);
    }
    return this;
}

/* ffe::Octree */

Octree::Octree():
    m_root(*this)
{

}

OctreeNode *Octree::insert_object(OctreeObject *obj)
{
    assert(!obj->m_parent);
    return m_root.insert_object(obj);
}

void Octree::remove_object(OctreeObject *obj)
{
    if (!obj->m_parent || &obj->m_parent->tree() != this) {
        return;
    }
    obj->m_parent->remove_object(obj);
}

}
