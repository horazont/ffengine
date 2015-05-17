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
    explicit OctreeNode(Octree &root, OctreeNode *parent = nullptr);

    OctreeNode(const OctreeNode &ref) = delete;
    OctreeNode &operator=(const OctreeNode &ref) = delete;
    OctreeNode(OctreeNode &&src) = delete;
    OctreeNode &operator=(OctreeNode &&src) = delete;

private:
    Octree &m_root;
    const OctreeNode *const m_parent;

    AABB m_bounds;
    bool m_bounds_valid;

    bool m_is_split;

    std::array<SplitPlane, 3> m_split_planes;

    std::array<std::unique_ptr<OctreeNode>, 8> m_children;
    std::vector<OctreeObject*> m_objects;

protected:
    OctreeNode &autocreate_child(unsigned int i);
    unsigned int find_child_for(const OctreeObject *obj);
    OctreeNode *insert_object(OctreeObject *obj);
    bool merge();
    void remove_object(OctreeObject *obj);
    bool split();

public:
    inline const OctreeNode *parent() const
    {
        return m_parent;
    }

public:
    container_type::iterator begin();
    container_type::const_iterator cbegin() const;
    container_type::reverse_iterator rbegin();
    container_type::const_reverse_iterator crbegin() const;
    container_type::iterator end();
    container_type::const_iterator cend() const;
    container_type::reverse_iterator rend();
    container_type::const_reverse_iterator crend() const;

    container_type::iterator erase(container_type::iterator el);
    container_type::iterator erase(container_type::iterator first,
                                   container_type::iterator last);

    friend class Octree;
};


class Octree
{
public:
    Octree();

private:
    OctreeNode m_root;

public:
    OctreeNode &root();
    const OctreeNode &root() const;

    OctreeNode *insert_object(OctreeObject *obj);
    void remove_object(OctreeObject *obj);

};


}

#endif
