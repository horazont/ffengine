/**********************************************************************
File name: octree.hpp
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
#ifndef SCC_ENGINE_RENDER_OCTREE_H
#define SCC_ENGINE_RENDER_OCTREE_H

#include <array>
#include <memory>
#include <vector>

#include "ffengine/math/shapes.hpp"


namespace engine {

class Octree;
class OctreeNode;

class OctreeObject
{
public:
    OctreeObject();
    ~OctreeObject();

private:
    OctreeNode *m_parent;
    Sphere m_bounding_sphere;

protected:
    void update_bounds(Sphere new_bounds);

public:
    const Octree *octree() const;
    Octree *octree();

    friend class Octree;
    friend class OctreeNode;
};


class OctreeNode
{
public:
    static const unsigned int SPLIT_THRESHOLD;
    static const unsigned int STRADDLE_THRESHOLD_DIVISOR;
    static const unsigned int CHILD_SELF = 8;

    typedef std::vector<OctreeObject*> container_type;

private:
    struct SplitPlane
    {
        bool enabled;
        Plane plane;
    };

public:
    explicit OctreeNode(Octree &tree);
    OctreeNode(Octree &tree,
               OctreeNode *parent,
               unsigned int index);

    OctreeNode(const OctreeNode &ref) = delete;
    OctreeNode &operator=(const OctreeNode &ref) = delete;
    OctreeNode(OctreeNode &&src) = delete;
    OctreeNode &operator=(OctreeNode &&src) = delete;

    ~OctreeNode();

private:
    Octree &m_tree;
    OctreeNode *const m_parent;
    const unsigned int m_index_at_parent;

    mutable AABB m_bounds;
    mutable bool m_bounds_valid;

    bool m_is_split;
    unsigned int m_nonempty_children;

    std::array<SplitPlane, 3> m_split_planes;

    std::array<std::unique_ptr<OctreeNode>, 8> m_children;
    std::vector<OctreeObject*> m_objects;

protected:
    OctreeNode &autocreate_child(unsigned int i);
    const AABB &updated_bounds() const;
    void delete_if_empty();
    unsigned int find_child_for(const OctreeObject *obj);
    OctreeNode *insert_object(OctreeObject *obj);
    bool merge();
    void notify_empty_child(unsigned int index);
    void remove_object(OctreeObject *obj);
    bool split();

public:
    inline const OctreeNode *parent() const
    {
        return m_parent;
    }

    inline Octree &tree()
    {
        return m_tree;
    }

    inline AABB bounds() const
    {
        return updated_bounds();
    }

    inline bool is_split() const
    {
        return m_is_split;
    }

    inline OctreeNode *child(unsigned int i)
    {
        return m_children[i].get();
    }

public:
    container_type::iterator begin();
    container_type::const_iterator cbegin() const
    {
        return m_objects.cbegin();
    }

    container_type::reverse_iterator rbegin();
    container_type::const_reverse_iterator crbegin() const;
    container_type::iterator end();

    container_type::const_iterator cend() const
    {
        return m_objects.cend();
    }

    container_type::reverse_iterator rend();
    container_type::const_reverse_iterator crend() const;

    inline container_type::size_type size() const
    {
        return m_objects.size();
    }

    friend class Octree;
};


class Octree
{
public:
    Octree();

private:
    OctreeNode m_root;

public:
    inline OctreeNode &root()
    {
        return m_root;
    }

    inline const OctreeNode &root() const
    {
        return m_root;
    }

    OctreeNode *insert_object(OctreeObject *obj);
    void remove_object(OctreeObject *obj);

};


}

#endif
