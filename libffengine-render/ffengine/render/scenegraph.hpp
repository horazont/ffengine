/**********************************************************************
File name: scenegraph.hpp
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
#ifndef SCC_ENGINE_SCENEGRAPH_H
#define SCC_ENGINE_SCENEGRAPH_H

#include <stack>
#include <vector>

#include "ffengine/common/types.hpp"
#include "ffengine/common/utils.hpp"

#include "ffengine/math/matrix.hpp"
#include "ffengine/math/quaternion.hpp"
#include "ffengine/math/octree.hpp"

#include "ffengine/gl/ibo.hpp"
#include "ffengine/gl/vao.hpp"

#include "ffengine/render/camera.hpp"

namespace ffe {

class RenderContext;

class RenderableOctreeObject: public ffe::OctreeObject
{
public:
    virtual void prepare(RenderContext &context) = 0;
    virtual void render(RenderContext &context) = 0;

};


namespace scenegraph {

/**
 * A scenegraph node, from which all nodes in the scenegraph inherit.
 *
 * It provides three methods, advance(), render() and sync(), and introduces
 * the concept of GPU-only storage.
 *
 * The engine is made for rendering within QML and Qt Quick, which means that
 * we have to take care with threading. In QML, there are two main threads:
 * one for handling UI events and one for rendering using OpenGL. When a
 * frame starts, the threads synchronize, by blocking the UI thread and letting
 * the OpenGL thread prepare the frame. When the frame is prepared, the UI
 * thread gets unblocked and the OpenGL thread starts to render. Read more
 * about this in the [Qt 5 manual](http://doc.qt.io/qt-5/qtquick-visualcanvas-scenegraph.html#threaded-render-loop).
 *
 * During the synchronization phase, sync() is called on all nodes. A node can
 * thus be sure that only the rendering thread is running while sync() is
 * called, and it has access to the OpenGL context.
 *
 * When the synchronization phase is over, render() is called on all nodes
 * which were in the scene graph at the point sync() was called (parent nodes
 * take care of separating them from changes which happen after sync()). When
 * render() is called, the UI thread may be running and thus no unguarded
 * access to shared data is allowed.
 *
 * To avoid bringing down performance, access to shared data should be avoided
 * altogehter. Instead, during sync(), data required for rendering should be
 * copied into local buffers or OpenGL buffers, depending on the type of data.
 *
 * For the sake of communication, let us call the state which is solely used
 * by render() "GPU-only data".
 */
class Node
{
public:
    Node();
    virtual ~Node();

public:
    /**
     * Advance the state of the node.
     *
     * @param seconds The number of seconds passed since the last call to
     * advance.
     *
     * This is called within the GUI thread. It must not touch GPU-only data
     * or make calls to OpenGL.
     */
    virtual void advance(TimeInterval seconds);

    /**
     * Prepare all state data needed for rendering into render-only storage
     * for the given render context.
     *
     * @param context The render context for which to synchronize this node.
     *
     * Calling methods which do drawcalls here has undefined effects,
     * most likely they won’t appear anywhere because clearing of the buffers
     * happens *after* prepare() and sync(). The RenderContext is already
     * filled with camera-related information, such as the projection and the
     * view matrices, as well as the viewpoint.
     *
     * Work which is independent of the current view should be done in sync(),
     * which is called *before* this method is called.
     *
     * When this method is called, it is legal to access all memory available
     * to the view.
     *
     * @see RenderContext.get_storage
     * @see SceneStorage
     * @see sync()
     *
     * @opengl
     */
    virtual void prepare(RenderContext &context) = 0;

    /**
     * Render the node into the render context.
     *
     * @param context Current rendering context
     *
     * When this method is called, only GPU-only storage may be accessed.
     * Anything outside that needs to be copied into GPU-only storage when the
     * sync() method is called.
     *
     * No direct drawcalls must be made. Instead, drawcalls must be submitted
     * to the RenderContext using its methods. The drawcalls are executed at
     * an unspecified later point and in an order dependent on the material
     * settings.
     *
     * @opengl
     */
    virtual void render(RenderContext &context) = 0;

    /**
     * Synchronize all view-agnostic data needed for rendering this node
     * into render-only storage.
     *
     * Data which is dependent on the current view can be synchronized in the
     * prepare() method, which is called after sync() and possibly called
     * multiple times for different Scene and RenderContext instances.
     *
     * When this method is called, it is legal to access all memory available
     * to the view.
     *
     * @see prepare()
     */
    virtual void sync() = 0;

};


/**
 * Group multiple scenegraph Nodes into a single node.
 *
 * This class takes a variable amount of children, which may be added and
 * removed at any time from within the GUI thread.
 *
 * When nodes are added or removed, the GPU-only set of nodes which is about
 * to be rendered is not touched. See the respective function for possible
 * caveats.
 *
 * @see Node
 */
class Group: public Node
{
protected:
    typedef std::vector<std::unique_ptr<Node> > internal_container;

public:
    typedef DereferencingIterator<internal_container::iterator, Group> iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

public:
    Group();

private:
    std::vector<std::unique_ptr<Node> > m_locked_children;
    std::vector<std::unique_ptr<Node> > m_children;
    std::vector<Node*> m_to_render;

public:
    inline iterator begin()
    {
        return iterator(m_children.begin());
    }

    inline iterator end()
    {
        return iterator(m_children.end());
    }

    /**
     * Add a node to the group.
     *
     * It will be rendered after the next call to sync().
     *
     * @param node The scene graph node to be added. The Group takes ownership.
     */
    void add(std::unique_ptr<Node> &&node);

    /**
     * Return the node at the given *index*, range-checked.
     *
     * @throws std::out_of_range if the index is out of bounds
     * @param index Index of the node.
     * @return Reference to the node.
     */
    Node &at(const unsigned int index);

    /**
     * Create and add a node to the group. Takes the same arguments as *T* and
     * returns a reference to the new node. The new node will be rendered after
     * the next call to sync().
     *
     * @return Reference to the new node.
     */
    template <typename T, typename... arg_ts>
    T &emplace(arg_ts&&... args)
    {
        T *result = new T(std::forward<arg_ts>(args)...);
        m_children.emplace_back(result);
        return *result;
    }

    /**
     * Erase a single node from the group.
     *
     * The node may not be deleted immediately, depending on whether it is
     * currently being rendered. After the next call to sync(), it will have
     * been deleted.
     *
     * @param iter Iterator pointing at the node to remove
     * @return Iterator pointing to the node behind the node which was
     * removed.
     */
    iterator erase(iterator iter);

    /**
     * Erase multiple nodes from the group, in the half-open interval
     * ``[first, last)``.
     *
     * See erase() for details on the actual time of deletion.
     *
     * @param first First node to erase.
     * @param last Node after the last node to erase.
     * @return New iterator pointing to the node at which last pointed.
     * @see erase
     */
    iterator erase(iterator first, iterator last);

    /**
     * Number of nodes currently in the group.
     *
     * @return Number of nodes currently in the group.
     */
    inline std::size_t size() const
    {
        return m_children.size();
    }

    /**
     * Access a specific node.
     *
     * @param at Index of the node
     * @return A pointer to the node or std::nullptr if the index is out of
     * range.
     */
    Node *operator[](const unsigned int at);

    /**
     * Remove and return the node at the given iterator position.
     *
     * In contrast to erase(), this does not take care of keeping the node
     * alive if neccessary for rendering.
     *
     * You **must** keep the node alive until rendering finishes. If you want
     * to simply delete a node, use erase() instead, which will take care of
     * keeping the node alive until the current render finishes but logically
     * removing it from the scenegraph.
     *
     * @param iter The iterator pointing at the node to remove.
     * @return A tuple consisting of the removed node and the iterator pointing
     * to the node which was behind the removed node.
     */
    std::tuple<std::unique_ptr<Node>, iterator> pop(iterator iter);

public:
    /**
     * Advance all children of this group.
     *
     * This simply calls Node::advance() on all children.
     *
     * @param seconds Time since the last call to advance()
     */
    void advance(TimeInterval seconds) override;

    /**
     * Copy the current list of nodes into render-storage and call sync()
     * on all those nodes.
     *
     * @opengl
     */
    void sync() override;

    /**
     * Call prepare() on all children which were in the group when sync() was
     * called last.
     *
     * @param context The render context to prepare for.
     *
     * @opengl
     */
    void prepare(RenderContext &context) override;

    /**
     * Render all nodes which were in the group at the time sync() was
     * called last.
     *
     * This basically calls Node::render() on all those nodes.
     *
     * @param context The context to render in.
     *
     * @opengl
     */
    void render(RenderContext &context) override;

};


/**
 * The InvisibleGroup class represents a scene graph node group which does not
 * get rendered.
 *
 * This class is much more efficient than a Group, as it does not need to keep
 * children alive while rendering is in progress.
 *
 * The sync() and render() methods are no-ops. Only advance() is forwarded to
 * the children.
 */
class InvisibleGroup: public Node
{
public:
    InvisibleGroup();

private:
    std::vector<std::unique_ptr<Node> > m_children;

public:
    std::vector<std::unique_ptr<Node> > &children();
    const std::vector<std::unique_ptr<Node> > &children() const;

public:
    /**
     * Advance all children
     *
     * Calls Node::advance() on all children.
     *
     * @param seconds Time since the last call to advance()
     */
    void advance(TimeInterval seconds) override;

    /**
     * Do nothing
     *
     * @param context Ignored.
     *
     * @opengl
     */
    void prepare(RenderContext &context) override;

    /**
     * Do nothing
     *
     * @param context Ignored.
     *
     * @opengl
     */
    void render(RenderContext &context) override;

    /**
     * Do nothing.
     *
     * @opengl
     */
    void sync() override;
};

/**
 * A scenegraph node which has a single node as a child.
 *
 * This is meant as a baseclass for changing render context state for a
 * subtree. It is much more lightweight than a Group.
 */
class ParentNode: public Node
{
public:
    ParentNode();
    explicit ParentNode(std::unique_ptr<Node> &&child);

private:
    std::unique_ptr<Node> m_locked_child;
    std::unique_ptr<Node> m_child;
    Node *m_child_to_render;

public:
    /**
     * Return a pointer to the current child node.
     *
     * @return Pointer to the current child node
     */
    inline Node *child() const
    {
        return m_child.get();
    }

    /**
     * Swap the current child for a different one.
     *
     * You **must** keep the returned node alive until the next call to sync().
     * If you simply want to set a new child, use set_child().
     *
     * @param node The new child to adopt
     * @return The old child.
     */
    std::unique_ptr<Node> swap_child(std::unique_ptr<Node> &&node);

    /**
     * Replace the current child, deleting it.
     *
     * The old child might be kept alive until the next call to sync().
     *
     * @param node New child to adopt
     */
    void set_child(std::unique_ptr<Node> &&node);

    /**
     * Create a node and replace the current child with it.
     *
     * The deletion of the old child has the same semantics as with
     * set_child().
     *
     * @return A reference to the newly created child.
     */
    template <typename child_t, typename... arg_ts>
    child_t &emplace_child(arg_ts&&... args)
    {
        child_t *result = new child_t(std::forward<arg_ts>(args)...);
        set_child(std::unique_ptr<Node>(result));
        return *result;
    }

public:
    /**
     * Advance the child
     *
     * Calls Node::advance() on the child, if any.
     *
     * @param seconds Seconds since the last call to advance().
     */
    void advance(TimeInterval seconds) override;

    /**
     * Calls prepare() on the child which was present at the last call to
     * sync(), if any.
     *
     * @param context The RenderContext to synchronize for.
     *
     * @opengl
     */
    void prepare(RenderContext &context) override;

    /**
     * Render the child, if any.
     *
     * Renders the child which was present at the last call to sync(), if any.
     *
     * @param context Context to render in.
     *
     * @opengl
     */
    void render(RenderContext &context) override;

    /**
     * Store the currently present child in render storage and call sync() on
     * it, if a child is present.
     *
     * @opengl
     */
    void sync() override;

};

/**
 * Apply a transformation matrix on a whole subtree. The transformation matrix
 * defaults to identity.
 */
class Transformation: public ParentNode
{
public:
    Transformation();
    explicit Transformation(std::unique_ptr<Node> &&child);

private:
    Matrix4f m_transform;
    Matrix4f m_render_transform;

public:
    /**
     * Transformation matrix
     *
     * @return Return a reference to the current transformation matrix, for
     * manipulation and reading.
     */
    inline Matrix4f &transformation()
    {
        return m_transform;
    }

    /**
     * Transformation matrix read access
     *
     * @return Return a const reference to the current transformation matrix.
     */
    inline const Matrix4f &transformation() const
    {
        return m_transform;
    }

public:
    /**
     * Apply the transformation which was active when sync was called and
     * render the child.
     *
     * @param context Render context to use
     *
     * @opengl
     */
    void render(RenderContext &context) override;

    /**
     * Synchronize the current transformation for rendering into GPU-only data
     * storage.
     *
     * Call ParentNode::sync().
     *
     * @opengl
     */
    void sync() override;

};


class OctContext
{
public:
    typedef std::tuple<Quaternionf, Vector3f> Transformation;

public:
    OctContext();

private:
    std::vector<Transformation> m_transformation_stack;

public:
    void push_translation(const Vector3f &d);
    void push_rotation(const Quaternionf &q);

    inline const Vector3f &get_origin() const
    {
        return std::get<1>(m_transformation_stack.back());
    }

    inline const Quaternionf &get_orientation() const
    {
        return std::get<0>(m_transformation_stack.back());
    }

    inline Vector3f apply_to(const Vector3f &vec) const
    {
        return get_origin() + get_orientation().rotate(vec);
    }

    void pop_transform();

    void reset();

};


class OctNode
{
public:
    virtual ~OctNode();

public:
    /**
     * @see Node::advance()
     */
    virtual void advance(TimeInterval seconds);

    /**
     * Synchronize the view-agnostic data the Octree-Scenegraph-Node needs
     * for rendering into render-storage.
     *
     * It is expected that all renderables are placed in the given \a octree
     * during this call. The renderables must be RenderableOctreeObject
     * instancee.
     *
     * @param octree The octree in which the RenderableOctreeObject instances
     * shall be placed.
     * @param positioning The current positioning context.
     *
     * @opengl
     */
    virtual void sync(ffe::Octree &octree,
                      OctContext &positioning);


};


class OctGroup: public OctNode
{
protected:
    typedef std::vector<std::unique_ptr<OctNode> > internal_container;

public:
    typedef DereferencingIterator<internal_container::iterator, OctGroup> iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

public:
    OctGroup();

private:
    internal_container m_locked_children;
    internal_container m_children;

public:
    inline iterator begin()
    {
        return iterator(m_children.begin());
    }

    inline iterator end()
    {
        return iterator(m_children.end());
    }

    /**
     * Add a node to the group.
     *
     * It will be rendered after the next call to sync().
     *
     * @param node The scene graph node to be added. The OctGroup takes
     * ownership.
     */
    void add(std::unique_ptr<OctNode> &&node);

    /**
     * Return the node at the given `index`, range-checked.
     *
     * @throws std::out_of_range if the index is out of bounds
     * @param index Index of the node.
     * @return Reference to the node.
     */
    OctNode &at(const unsigned int index);

    /**
     * Create and add a node to the group. Takes the same arguments as `T` and
     * returns a reference to the new node. The new node will be rendered after
     * the next call to sync().
     *
     * @return Reference to the new node.
     */
    template <typename T, typename... arg_ts>
    T &emplace(arg_ts&&... args)
    {
        T *result = new T(std::forward<arg_ts>(args)...);
        m_children.emplace_back(result);
        return *result;
    }

    /**
     * Erase a single node from the group.
     *
     * The node may not be deleted immediately, depending on whether it is
     * currently being rendered. After the next call to sync(), it will have
     * been deleted.
     *
     * @param iter Iterator pointing at the node to remove
     * @return Iterator pointing to the node behind the node which was
     * removed.
     */
    iterator erase(iterator iter);

    /**
     * Erase multiple nodes from the group, in the half-open interval
     * ``[first, last)``.
     *
     * See erase() for details on the actual time of deletion.
     *
     * @param first First node to erase.
     * @param last OctNode after the last node to erase.
     * @return New iterator pointing to the node at which last pointed.
     * @see erase
     */
    iterator erase(iterator first, iterator last);

    /**
     * Number of nodes currently in the group.
     *
     * @return Number of nodes currently in the group.
     */
    inline std::size_t size() const
    {
        return m_children.size();
    }

    /**
     * Access a specific node.
     *
     * @param at Index of the node
     * @return A pointer to the node or std::nullptr if the index is out of
     * range.
     */
    OctNode *operator[](const unsigned int at);

    /**
     * Remove and return the node at the given iterator position.
     *
     * In contrast to erase(), this does not take care of keeping the node
     * alive if neccessary for rendering.
     *
     * You **must** keep the node alive until rendering finishes. If you want
     * to simply delete a node, use erase() instead, which will take care of
     * keeping the node alive until the current render finishes but logically
     * removing it from the scenegraph.
     *
     * @param iter The iterator pointing at the node to remove.
     * @return A tuple consisting of the removed node and the iterator pointing
     * to the node which was behind the removed node.
     */
    std::tuple<std::unique_ptr<OctNode>, iterator> pop(iterator iter);

public:
    void advance(TimeInterval seconds) override;

    void sync(ffe::Octree &octree,
              OctContext &positioning) override;

};


class OctParentNode: public OctNode
{
private:
    std::unique_ptr<OctNode> m_child;
    std::unique_ptr<OctNode> m_locked_child;

public:
    /**
     * Return a pointer to the current child node.
     *
     * @return Pointer to the current child node
     */
    inline OctNode *child() const
    {
        return m_child.get();
    }

    /**
     * Swap the current child for a different one.
     *
     * You **must** keep the returned node alive until the next call to sync().
     * If you simply want to set a new child, use set_child().
     *
     * @param node The new child to adopt
     * @return The old child.
     */
    std::unique_ptr<OctNode> swap_child(std::unique_ptr<OctNode> &&node);

    /**
     * Replace the current child, deleting it.
     *
     * The old child might be kept alive until the next call to sync().
     *
     * @param node New child to adopt
     */
    void set_child(std::unique_ptr<OctNode> &&node);

    /**
     * Create a node and replace the current child with it.
     *
     * The deletion of the old child has the same semantics as with
     * set_child().
     *
     * @return A reference to the newly created child.
     */
    template <typename child_t, typename... arg_ts>
    child_t &emplace_child(arg_ts&&... args)
    {
        child_t *result = new child_t(std::forward<arg_ts>(args)...);
        set_child(std::unique_ptr<OctNode>(result));
        return *result;
    }

public:
    void advance(TimeInterval seconds) override;
    void sync(ffe::Octree &octree,
              OctContext &positioning) override;

};


class OctRotation: public OctParentNode
{
public:
    OctRotation() = default;
    explicit OctRotation(const Quaternionf &q);

private:
    Quaternionf m_rotation;

public:
    inline void set_rotation(Quaternionf &&src)
    {
        m_rotation = src;
    }

    inline const Quaternionf &rotation() const
    {
        return m_rotation;
    }

public:
    void sync(ffe::Octree &octree,
              OctContext &positioning) override;

};


class OctTranslation: public OctParentNode
{
public:
    OctTranslation() = default;
    explicit OctTranslation(const Vector3f &d);

private:
    Vector3f m_translation;

public:
    inline void set_translation(Vector3f &&src)
    {
        m_translation = src;
    }

    inline const Vector3f &translation() const
    {
        return m_translation;
    }

public:
    void sync(ffe::Octree &octree,
              OctContext &positioning) override;

};


class OctreeGroup: public Node
{
private:
    OctGroup m_root;
    ffe::Octree m_octree;

    // used during sync
    OctContext m_positioning;
    std::vector<ffe::OctreeNode*> m_hitset;

    std::unordered_map<RenderContext*, std::vector<RenderableOctreeObject*> > m_to_render;

    std::atomic_uint_least32_t m_selected_objects;

public:
    inline OctGroup &root()
    {
        return m_root;
    }

    inline const OctGroup &root() const
    {
        return m_root;
    }

    inline ffe::Octree &octree()
    {
        return m_octree;
    }

    inline const ffe::Octree &octree() const
    {
        return m_octree;
    }


    inline uint_least32_t selected_objects() const
    {
        return m_selected_objects;
    }

public:
    void advance(TimeInterval seconds) override;
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;
    void sync() override;

};


} /* namespace scenegraph */


class SceneGraph
{
public:
    SceneGraph();

private:
    Vector4f m_sun_colour;
    Vector3f m_sun_direction;
    Vector4f m_sky_colour;

    scenegraph::Group m_root;

public:
    inline scenegraph::Group &root()
    {
        return m_root;
    }

    inline const scenegraph::Group &root() const
    {
        return m_root;
    }

    inline void set_sky_colour(const Vector4f &colour)
    {
        m_sky_colour = colour;
    }

    inline void set_sun_colour(const Vector4f &colour)
    {
        m_sun_colour = colour;
    }

    inline void set_sun_direction(const Vector3f &dir)
    {
        m_sun_direction = dir;
    }

    inline const Vector4f &sky_colour() const
    {
        return m_sky_colour;
    }

    inline const Vector4f &sun_colour() const
    {
        return m_sun_colour;
    }

    inline const Vector3f &sun_direction() const
    {
        return m_sun_direction;
    }

public:
    inline void advance(TimeInterval seconds)
    {
        m_root.advance(seconds);
    }

    void prepare(RenderContext &context);
    void render(RenderContext &context);
    void sync();

};

} /* namespace engine */

#endif
