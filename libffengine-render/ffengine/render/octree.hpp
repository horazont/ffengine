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

/**
 * Base class for objects which can be inserted into Octree instances.
 *
 * These objects are bounded by a Sphere for sorting them into the Octree.
 *
 * Upon destruction, OctreeObject instances remove themselves from the Octree
 * they are associated with. See Octree.remove_object() for possible
 * side-effects.
 */
class OctreeObject
{
public:
    /**
     * Construct a new OctreeObject. The bounding sphere is set to a
     * zero-sized sphere centered at (0, 0, 0).
     */
    OctreeObject();
    ~OctreeObject();

private:
    OctreeNode *m_parent;
    Sphere m_bounding_sphere;

protected:
    /**
     * Update the bounding sphere and re-insert the object into the Octree.
     * To update the position and bounds within the octree, the object is
     * re-inserted. See the respective Octree methods for side-effects of that.
     *
     * @param new_bounds New bounding sphere.
     *
     * @see Octree.insert_object()
     * @see Octree.remove_object()
     */
    void update_bounds(Sphere new_bounds);

public:
    const Octree *octree() const;
    Octree *octree();

    friend class Octree;
    friend class OctreeNode;
};

/**
 * Record information about ray hits in the ray query result.
 *
 * @see Octree.select_nodes_by_ray()
 */
struct OctreeRayHitInfo
{
    /**
     * The OctreeNode which was hit by the ray.
     */
    OctreeNode *node;

    /**
     * The t value along the ray when it entered the node.
     */
    float tmin;

    /**
     * The t value along the ray when it exited the node.
     */
    float tmax;
};


/**
 * A node in a Dynamic Irregular Octree, as described by Shagam et al.
 *
 * Each OctreeNode may split when more than SPLIT_THRESHOLD objects are
 * contained in the node. When a split happens, the mean of the node is
 * calculated by taking the weighted average of the centers of the bounding
 * spheres of the objects. Each sphere center is weighted with the inverse
 * of the spheres radius, to bias the mean towards clusters of small objects.
 *
 * The split planes, along each pair of axis, are then located at that mean
 * point. If too many objects still intersect with the planes, up to one plane
 * is disabled, effectively degrading the Octree to a Quad- or KD-Tree.
 *
 * Objects are sorted into the deepest node where they do not intersect with
 * any of the enabled planes. Thus, objects are at most in one node at a time.
 *
 * Any operation removing from, inserting to or moving objects within the tree
 * can render pointers to nodes invalid. Nodes are destroyed if they contain
 * neither children nor objects, which may in turn cause the parent node to
 * become destroyed. The only node exempt from this rule is the root node.
 *
 * To insert or remove objects, the methods on the Octree class must be used
 * (Octree.insert_object(), Octree.remove_object()).
 *
 * To move objects, the objects must update their position using
 * OctreeObject.update_bounds().
 *
 * @see Octree
 * @see Octree.insert_object()
 * @see Octree.remove_object()
 * @see OctreeObject.update_bounds()
 */
class OctreeNode
{
public:
    static const unsigned int SPLIT_THRESHOLD;
    static const unsigned int STRADDLE_THRESHOLD_DIVISOR;
    static const unsigned int CHILD_SELF = 8;

    typedef std::vector<OctreeObject*> container_type;

private:
    /**
     * Hold information about a splitting plane.
     */
    struct SplitPlane
    {
        /**
         * Whether the plane is currently used for splitting.
         */
        bool enabled;

        /**
         * The actual plane used for splitting.
         */
        Plane plane;
    };

protected:
    /**
     * Create a new root OctreeNode.
     *
     * The associated tree cannot be changed during a nodes lifetime.
     *
     * @param tree The Octree instance which holds the tree.
     */
    explicit OctreeNode(Octree &tree);

    /**
     * Create a new child OctreeNode.
     *
     * Neither the parent nor the tree nor the index can be changed during a
     * nodes lifetime.
     *
     * @param parent The parent OctreeNode.
     * @param index Index of the new node inside the parent OctreeNode.
     */
    OctreeNode(OctreeNode &parent, unsigned int index);

public:
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
    /**
     * Return the \a i th child. If the child does not exist currently, create
     * it and return the newly created child.
     *
     * @param i Index of the child; must be greater than or equal to zero and
     * less than 8.
     * @return Child object.
     */
    OctreeNode &autocreate_child(unsigned int i);

    /**
     * Return a const reference to the AABB bounding the OctreeNode.
     *
     * If the cached bounds are not recent (indicated by an internal flag),
     * the bounds are recalculated internally.
     *
     * This is an implementation detail; in theory, the bounds could be
     * calculated on each call to updated_bounds(), which is why the method
     * is still const.
     *
     * @return const reference to an AABB describing the outer bounds of the
     * node and all of its children and objects.
     */
    const AABB &updated_bounds() const;

    /**
     * Check if the node is empty and if it is, notify the parent node about
     * that fact.
     *
     * If no parent node exists, this is a no-op. Otherwise, the parent will
     * delete the node and possibly itself.
     */
    void delete_if_empty();

    /**
     * Find the index of the child node to store the given object in.
     *
     * This does not check whether the node is split, but uses the currently
     * defined split planes.
     *
     * @param obj OctreeObject for which the child node must be found
     * @return \l CHILD_SELF if the object intersects with any of the planes.
     * An index between 0 (incl.) and 8 (excl.) otherwise. The child may not
     * exist and should be accessed using autocreate_child().
     */
    unsigned int find_child_for(const OctreeObject *obj);

    /**
     * Insert an object into this node or a child node.
     *
     * @param obj OctreeObject to insert
     * @return The OctreeNode which finally got the object.
     */
    OctreeNode *insert_object(OctreeObject *obj);

    /**
     * Merge the node.
     *
     * A node may only be merged if all of its child nodes are merged or do
     * not exist.
     *
     * If the node is not split, return true immediately.
     *
     * @return true if the node could be merged, false otherwise.
     */
    bool merge();

    /**
     * Notify that the child at \a index is empty.
     *
     * This deletes the child and, if no other children and no objects are
     * present, possibly also this node.
     *
     * @param index The index of the child.
     */
    void notify_empty_child(unsigned int index);

    /**
     * Remove an object from this node.
     *
     * If the object is not in this node, this is an (expensive) no-op.
     *
     * @param obj The object to remove.
     */
    void remove_object(OctreeObject *obj);

    /**
     * Select nodes by testing whether they intersect the given Ray and
     * contain objects.
     *
     * Empty nodes are never returned, but their child nodes may be returned
     * if they intersect the ray.
     *
     * @param r The Ray to test against
     * @param hitset A vector to store the hit results.
     *
     * @see OctreeRayHitInfo
     * @see Octree.select_nodes_by_ray()
     */
    void select_nodes_by_ray(const Ray &r,
                             std::vector<OctreeRayHitInfo> &hitset);

    /**
     * Split the node.
     *
     * If the node is already split, return true immediately.
     *
     * @return true if the split was successful, false otherwise. A split
     * may fail if too few objects are in the node. The minimum number of
     * objects to perfom a split is an implementation detail, but it must be
     * at least one; an empty node will thus never split.
     */
    bool split();

public:
    /**
     * Access the parent node.
     */
    inline const OctreeNode *parent() const
    {
        return m_parent;
    }

    /**
     * Access the Octree to which this node belongs.
     */
    inline Octree &tree()
    {
        return m_tree;
    }

    /**
     * Return the bounds covering this node and all of its children.
     */
    inline AABB bounds() const
    {
        return updated_bounds();
    }

    /**
     * Return true if this node is currently split and false otherwise.
     */
    inline bool is_split() const
    {
        return m_is_split;
    }

    /**
     * Return a pointer to the \a i th child of this node.
     *
     * The pointer may be nullptr, even if the node is split, as empty
     * children are not created (and deleted if not needed anymore).
     *
     * @param i Index of the child. Must be a number between 0 (incl.) and 8
     * (excl.).
     * @return Pointer to the child.
     */
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

    /**
     * Return the number of objects in this node (excluding child nodes).
     */
    inline container_type::size_type size() const
    {
        return m_objects.size();
    }

    friend class Octree;
};


/**
 * A spatial access acceleration structure, based on Dynamic Irregular Octrees
 * by Shagam et. al..
 *
 * Implementation details are described in the OctreeNode class. The Octree
 * class provides the public interface.
 *
 * There is no requirement to provide outer bounds for an octree, as the
 * splitting planes for separating child nodes of a node are chosen
 * dynamically.
 */
class Octree
{
public:
    /**
     * Construct a new, empty octree.
     */
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

    /**
     * Insert an OctreeObject into the octree.
     *
     * The object is inserted using its current internal bounding sphere.
     *
     * Inserting an object into the tree invalidates all pointers to tree
     * nodes, as the tree may arbitrarily reorganize itself. There may be ways
     * to gracefully handle this in future versions.
     *
     * @param obj The object to insert
     * @return
     */
    OctreeNode *insert_object(OctreeObject *obj);

    /**
     * Remove an object from the octree.
     *
     * If the object is not part of the octree, this is a no-op.
     *
     * Removing an object from the tree invalidates all pointers to tree
     * nodes, as the tree may arbitrarily reorganize itself. There may be
     * ways to handle this gracefully in future versions.
     *
     * @param obj The object to remove.
     */
    void remove_object(OctreeObject *obj);

    /**
     * Select octree nodes using a ray intersection test.
     *
     * Only those octree nodes which both intersect the given Ray \a r and
     * directly contain objects are selected. Thus, parent nodes without
     * objects of non-empty leaves are not contained in the hit set.
     *
     * @param r The ray to use for the ray intersection test
     * @param hitset A vector to store the
     */
    void select_nodes_by_ray(
            const Ray &r,
            std::vector<OctreeRayHitInfo> &hitset);
};


}

#endif
