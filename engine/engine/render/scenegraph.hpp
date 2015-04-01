#ifndef SCC_ENGINE_SCENEGRAPH_H
#define SCC_ENGINE_SCENEGRAPH_H

#include <stack>
#include <vector>

#include "engine/math/matrix.hpp"

#include "engine/gl/ibo.hpp"
#include "engine/gl/material.hpp"
#include "engine/gl/vao.hpp"


namespace engine {
namespace scenegraph {

typedef float TimeInterval;


class DrawRequest
{
public:
    DrawRequest(VAO &arrays, Material &material);

protected:
    VAO &m_arrays;
    Material &m_material;

public:
    VAO &arrays();
    Material &material();

};


class BasicDrawRequest
{
public:
    BasicDrawRequest(VAO &arrays, Material &material);
};


class RenderContext
{
public:
    static constexpr GLint MATRIX_BLOCK_UBO_SLOT = 0;
    typedef UBO<Matrix4f, Matrix4f, Matrix4f, Matrix3f> MatrixUBO;

public:
    RenderContext();

private:
    MatrixUBO m_matrix_ubo;
    std::stack<Matrix4f> m_model_stack;
    Matrix4 m_current_transformation;

public:
    void draw_elements(GLenum primitive,
                       VAO &with_arrays, Material &using_material,
                       IBOAllocation &indicies);

    void pop_transformation();

    void push_transformation(const Matrix4f &mat);

public:
    Matrix4f projection();
    Matrix4f view();

    void set_projection(const Matrix4f &proj);
    void set_view(const Matrix4f &view);

};


class Node
{
public:
    Node();
    virtual ~Node();

public:
    /**
     * @brief Advance the state of the node (e.g. animation).
     * @param seconds The number of seconds passed since the last call to
     * advance.
     *
     * This is called within the GUI thread. It must not touch any OpenGL state
     * or make any calls to OpenGL.
     */
    virtual void advance(TimeInterval seconds);

    /**
     * @brief Render the node.
     * @param context Current rendering context
     *
     * When this method is called, only GPU-only storage may be accessed.
     * Anything outside that needs to be copied into GPU-only storage when the
     * sync() method is called.
     */
    virtual void render(RenderContext &context) = 0;

    /**
     * @brief Synchronize the node state to the GPU or other storage.
     *
     * When this method is called, it is legal to access all memory available
     * to the view.
     */
    virtual void sync() = 0;

};


class Group: public Node
{
protected:
    typedef std::vector<std::unique_ptr<Node> > internal_container;

public:
    template <typename internal_iterator>
    class iterator_tpl
    {
    public:
        typedef std::bidirectional_iterator_tag iterator_category;
        typedef typename internal_iterator::difference_type difference_type;
        typedef Node& reference;
        typedef Node*& pointer;
        typedef Node& value_type;

    public:
        iterator_tpl(internal_iterator curr):
            m_curr(curr)
        {

        }

    private:
        internal_iterator m_curr;

    public:
        inline bool operator==(const iterator_tpl &other) const
        {
            return (m_curr == other.m_curr);
        }

        inline bool operator!=(const iterator_tpl &other) const
        {
            return (m_curr != other.m_curr);
        }

        inline iterator_tpl operator++(int) const
        {
            return iterator(std::next(m_curr));
        }

        inline iterator_tpl &operator++()
        {
            ++m_curr;
            return *this;
        }

        inline iterator_tpl operator--(int) const
        {
            return iterator(std::prev(m_curr));
        }

        inline iterator_tpl &operator--()
        {
            --m_curr;
            return *this;
        }

        inline Node &operator*() const
        {
            return **m_curr;
        }

        inline Node &operator->() const
        {
            return **m_curr;
        }

        friend class Group;
    };

    typedef iterator_tpl<internal_container::iterator> iterator;
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

    void add(std::unique_ptr<Node> &&node);
    Node &at(const unsigned int index);

    template <typename T, typename... arg_ts>
    T &emplace(arg_ts&&... args)
    {
        T *result = new T(std::forward<arg_ts>(args)...);
        m_children.emplace_back(result);
        return *result;
    }

    iterator erase(iterator iter);
    iterator erase(iterator first, iterator last);

    inline std::size_t size() const
    {
        return m_children.size();
    }

    Node *operator[](const unsigned int at);

    /**
     * @brief Remove and return the node at the given iterator position.
     * @param iter The iterator pointing at the node to remove.
     * @return A tuple consisting of the removed node and the iterator pointing
     * to the node which was behind the removed node.
     *
     * You **must** keep the node alive until rendering finishes. If you want
     * to simply delete a node, use erase() instead, which will take care of
     * keeping the node alive until the current render finishes but logically
     * removing it from the scenegraph.
     */
    std::tuple<std::unique_ptr<Node>, iterator> pop(iterator iter);

public:
    /**
     * @brief Advance all children of this group
     * @param seconds Time since the last call to advance()
     */
    void advance(TimeInterval seconds) override;

    /**
     * @brief Render all children which were in the group at the time sync()
     * was called last.
     * @param context The context to render in.
     */
    void render(RenderContext &context) override;

    /**
     * @brief Synchronize all children currently in the group for the next
     * call to render().
     */
    void sync() override;

};


/**
 * @brief The InvisibleGroup class represents a scene graph node group which
 * does not get rendered.
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
     * @brief Advance all children
     * @param seconds Time since the last call to advance()
     */
    void advance(TimeInterval seconds) override;

    /**
     * @brief Do nothing
     * @param context Ignored.
     */
    void render(RenderContext &context) override;

    /**
     * @brief Do nothing
     */
    void sync() override;

};


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
     * @brief Return a pointer to the current child node
     * @return Pointer to the current child node
     */
    inline Node *child() const
    {
        return m_child.get();
    }

    /**
     * @brief Swap the current child for a different one
     * @param node The new child to adopt
     * @return The old child.
     *
     * You **must** keep the returned node alive until the next call to sync().
     * If you simply want to set a new child, use set_child().
     */
    std::unique_ptr<Node> swap_child(std::unique_ptr<Node> &&node);

    /**
     * @brief Replace the current child, deleting it.
     * @param node New child to adopt
     *
     * The old child is deleted when the next call to sync() happens.
     */
    void set_child(std::unique_ptr<Node> &&node);

    template <typename child_t, typename... arg_ts>
    child_t &emplace_child(arg_ts&&... args)
    {
        child_t *result = new child_t(std::forward<arg_ts>(args)...);
        set_child(std::unique_ptr<Node>(result));
        return *result;
    }

public:
    void advance(TimeInterval seconds) override;
    void render(RenderContext &context) override;
    void sync() override;

};


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
     * @brief Transformation matrix
     * @return Return a reference to the current transformation matrix, for
     * manipulation and reading.
     */
    inline Matrix4f &transformation()
    {
        return m_transform;
    }

    inline const Matrix4f &transformation() const
    {
        return m_transform;
    }

public:
    /**
     * @brief Apply the transformation which was active when sync was called
     * and render the child.
     * @param context Render context to use
     */
    void render(RenderContext &context) override;

    /**
     * @brief Synchronize the current transformation for rendering.
     */
    void sync() override;

};


class CameraPlaneController
{
public:
    CameraPlaneController();

private:
    Vector2f m_pos;
    Vector2f m_pos_vel;
    Vector2f m_pos_accel;

    Vector2f m_rot;
    Vector2f m_rot_vel;
    Vector2f m_rot_accel;

    float m_distance;
    float m_distance_vel;
    float m_distance_accel;

    bool m_moving, m_rotating;

public:
    inline const Vector2f &pos() const
    {
        return m_pos;
    }

    inline const Vector2f &rot() const
    {
        return m_rot;
    }

    inline const float &distance() const
    {
        return m_distance;
    }

    void set_pos(const Vector2f &pos, bool reset_mechanics = true);
    void set_rot(const Vector2f &rot, bool reset_mechanics = true);
    void set_distance(const float distance, bool reset_mechanics = true);

    void boost_movement(const Vector2f &by);
    void boost_rotation(const Vector2f &by);
    void boost_zoom(const float by);

    void stop_all();

public:
    void advance(TimeInterval seconds);

};


/**
 * @brief The Camera class
 *
 * A camera is responsible for setting up both the projection and the view
 * matrices in a RenderContext. When a scene graph is rendered, a Camera needs
 * to be specified which will set up the basic RenderContext.
 */
class Camera
{
public:
    Camera();
    virtual ~Camera();

protected:
    Matrix4f m_render_projection;
    Matrix4f m_render_view;

public:
    virtual void advance(TimeInterval seconds);
    void configure_context(RenderContext &context);
    virtual void sync() = 0;

};


class OrthogonalCamera: public Camera
{
public:
    OrthogonalCamera(
            float viewport_width,
            float viewport_height);

private:
    CameraPlaneController m_controller;

    float m_viewport_width;
    float m_viewport_height;
    float m_znear;
    float m_zfar;

    Matrix4f m_projection;

protected:
    void update_projection();

public:
    inline float viewport_height() const
    {
        return m_viewport_height;
    }

    inline float viewport_width() const
    {
        return m_viewport_width;
    }

    inline float zfar() const
    {
        return m_zfar;
    }

    inline float znear() const
    {
        return m_znear;
    }

    inline CameraPlaneController &controller()
    {
        return m_controller;
    }

public:
    void set_viewport(const float width, const float height);
    void set_znear(const float znear);
    void set_zfar(const float zfar);

public:
    void advance(TimeInterval seconds) override;
    void sync() override;

};


class PerspectivalCamera: public Camera
{
public:
    PerspectivalCamera();

private:
    CameraPlaneController m_controller;

    float m_viewport_width;
    float m_viewport_height;
    float m_znear;
    float m_zfar;

    float m_fovy;

    Matrix4f m_projection;

protected:
    void update_projection();

public:
    inline float viewport_height() const
    {
        return m_viewport_height;
    }

    inline float viewport_width() const
    {
        return m_viewport_width;
    }

    inline float zfar() const
    {
        return m_zfar;
    }

    inline float znear() const
    {
        return m_znear;
    }

    inline float fovy() const
    {
        return m_fovy;
    }

    inline CameraPlaneController &controller()
    {
        return m_controller;
    }

public:
    void set_fovy(const float fovy);
    void set_viewport(const float width, const float height);
    void set_znear(const float znear);
    void set_zfar(const float zfar);

public:
    void advance(TimeInterval seconds) override;
    void sync() override;

};


class SceneGraph
{
public:
    SceneGraph();

private:
    RenderContext m_render_context;
    Group m_root;

public:
    inline Group &root()
    {
        return m_root;
    }

    inline const Group &root() const
    {
        return m_root;
    }

    inline RenderContext &render_context()
    {
        return m_render_context;
    }

    inline const RenderContext &render_context() const
    {
        return m_render_context;
    }

public:
    inline void advance(TimeInterval seconds)
    {
        m_root.advance(seconds);
    }

    void render(Camera &camera);

    inline void sync()
    {
        m_root.sync();
    }

};


}
}

#endif
