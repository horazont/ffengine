/**********************************************************************
File name: rendergraph.hpp
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
#ifndef SCC_ENGINE_RENDERGRAPH_H
#define SCC_ENGINE_RENDERGRAPH_H

#include <array>
#include <vector>

#include "ffengine/gl/vao.hpp"
#include "ffengine/gl/material.hpp"
#include "ffengine/gl/fbo.hpp"
#include "ffengine/math/shapes.hpp"
#include "ffengine/math/vector.hpp"


namespace engine {

typedef Vector<unsigned int, 2> ViewportSize;


class SceneGraph;
class Camera;


class SceneStorage
{
public:
    virtual ~SceneStorage();

};


/**
 * Track the environment in which a render takes place.
 */
class RenderContext
{
public:
    static constexpr GLint MATRIX_BLOCK_UBO_SLOT = 0;
    typedef UBO<Matrix4f, Matrix4f, Matrix4f, Matrix3f> MatrixUBO;
    static constexpr GLint INV_MATRIX_BLOCK_UBO_SLOT = 1;
    typedef UBO<Matrix4f, Matrix4f> InvMatrixUBO;

public:
    RenderContext(SceneGraph &scenegraph, Camera &camera);

private:
    SceneGraph &m_scenegraph;
    Camera &m_camera;

    Vector3f m_render_viewpoint;
    Matrix4f m_render_view;

    std::unordered_map<void*, std::unique_ptr<SceneStorage> > m_storage;

    GLsizei m_viewport_width, m_viewport_height;
    GLfloat m_zfar, m_znear;

    MatrixUBO m_matrix_ubo;
    InvMatrixUBO m_inv_matrix_ubo;
    std::vector<Matrix4f> m_model_stack;
    Matrix4f m_current_transformation;

    std::array<Plane, 4> m_frustum;

    Vector3f m_viewpoint;

protected:
    void prepare_draw();

public:
    void draw_elements(GLenum primitive,
                       VAO &with_arrays, Material &using_material,
                       IBOAllocation &indicies);
    void draw_elements_less(GLenum primitive,
                            VAO &with_arrays, Material &using_material,
                            IBOAllocation &indicies,
                            unsigned int nmax);
    void draw_elements_base_vertex(GLenum primitive,
                                   VAO &with_arrays, Material &using_material,
                                   IBOAllocation &indicies,
                                   GLint base_vertex);

    void pop_transformation();

    void push_transformation(const Matrix4f &mat);

    void reset();

    void start();

public:
    inline SceneGraph &scenegraph()
    {
        return m_scenegraph;
    }

    inline Camera &camera()
    {
        return m_camera;
    }

    inline Vector3f &viewpoint()
    {
        return m_render_viewpoint;
    }

    inline Matrix4f &view()
    {
        return m_render_view;
    }

    inline GLsizei viewport_width() const
    {
        return m_viewport_width;
    }

    inline GLsizei viewport_height() const
    {
        return m_viewport_height;
    }

    inline GLfloat znear() const
    {
        return m_znear;
    }

    inline GLfloat zfar() const
    {
        return m_zfar;
    }

    inline const std::array<Plane, 4> &frustum() const
    {
        return m_frustum;
    }

public:
    void set_render_view(const Matrix4f &view);
    void set_render_viewpoint(const Vector3f &viewpoint);
    void set_viewport_size(GLsizei viewport_width, GLsizei viewport_height);

public:
    /**
     * Create and return the scene level storage for the given \a for_object.
     *
     * The storage is indexed using the objects pointer. If no storage has
     * been allocated yet for \a for_object, a new object of type \a T is
     * allocated, added to the internal map and returned.
     *
     * Otherwise, the existing object is dynamically casted to \a T and
     * returned.
     */
    template <typename T>
    T &get_storage(void *for_object)
    {
        static_assert(std::is_base_of<SceneStorage, T>::value,
                      "Scene storage type must be subclass of SceneStorage");
        auto iter = m_storage.find(for_object);
        if (iter == m_storage.end()) {
            T *storage = new T();
            m_storage[for_object] = std::unique_ptr<SceneStorage>(storage);
            return *storage;
        }

        return dynamic_cast<T&>(*iter->second);
    }

public:
    void sync();

public:
    /**
     * Configure a linked shader for use with RenderContext instances.
     *
     * This introspects the shaders uniform blocks and binds the UBOs of
     * RenderContexts which match the declarations to the corresponding
     * variables.
     *
     * @param shader A shader program to configure.
     */
    static void configure_shader(ShaderProgram &shader);

};


/**
 * Node in the rendergraph.
 *
 * A rendergraph node describes a step to achieve the finally rendered scene.
 * The activity is determined by the subclasses.
 *
 * Each render node has a RenderTarget attached on which it works.
 *
 * A render node can declare other render nodes as its dependencies, using the
 * vector returned by dependencies().
 */
class RenderNode
{
public:
    typedef std::vector<RenderNode*> container_type;

public:
    /**
     * Construct a rander node which renders into the given \a target.
     *
     * @param target RenderTarget to render into.
     */
    RenderNode(RenderTarget &target);
    virtual ~RenderNode();

protected:
    RenderTarget &m_target;
    std::vector<RenderNode*> m_dependencies;

public:
    /**
     * Dependencies of the render node, which are other render nodes.
     *
     * This list of nodes is used by the RenderGraph to determine the order
     * in which render nodes are executed.
     */
    inline container_type &dependencies()
    {
        return m_dependencies;
    }

    /**
     * @see \link dependencies() \endlink
     */
    inline const container_type &dependencies() const
    {
        return m_dependencies;
    }

public:
    virtual void render() = 0;
    virtual void sync() = 0;

};


/**
 * Blit a render target into another. This blits the Color and the Depth buffer
 * from the source into the destination.
 */
class BlitNode: public RenderNode
{
public:
    /**
     * Create a node which blits the buffer described by the \a src
     * RenderTarget into the \a dest target.
     *
     * \a dest is the \a target for the RenderNode.
     *
     * @param src Source
     * @param dest Destination
     */
    BlitNode(RenderTarget &src, RenderTarget &dest);

protected:
    RenderTarget &m_src;

public:
    void render() override;
    void sync() override;

};


/**
 * Render a SceneGraph with a Camera into the given target.
 */
class SceneRenderNode: public RenderNode
{
public:
    SceneRenderNode(RenderTarget &target,
                    SceneGraph &scenegraph,
                    Camera &camera);

protected:
    RenderContext m_context;

    GLbitfield m_clear_mask;
    Vector4f m_clear_colour;

public:
    inline GLbitfield clear_mask() const
    {
        return m_clear_mask;
    }

    inline const Vector4f &clear_colour() const
    {
        return m_clear_colour;
    }

public:
    /**
     * Define which parts of the buffer shall be cleared before rendering.
     *
     * @param mask Bitmask as passed to glClear
     */
    void set_clear_mask(GLbitfield mask);

    /**
     * Set the clear colour.
     *
     * This has no effect if GL_COLOR_BUFFER_BIT is not included in the mask
     * set using set_clear_mask().
     *
     * @param colour The colour to fill the buffer with before rendering.
     */
    void set_clear_colour(const Vector4f &colour);

public:
    void render() override;
    void sync() override;

};


/* class FBORenderNode: public RenderNode
{
public:
    FBORenderNode(SceneGraph &scenegraph, Camera &camera,
                  FBO &dest);

protected:
    FBO &m_dest;

public:
    void render() override;
    void sync() override;

}; */


/**
 * A render graph.
 *
 * The render graph describes the steps required to get the desired image onto
 * the users screen.
 *
 * The render graph consists of RenderNode instances, which are automatically
 * ordered using topological sort.
 */
class RenderGraph
{
public:
    RenderGraph();

private:
    std::vector<std::unique_ptr<RenderNode> > m_locked_nodes;
    std::vector<std::unique_ptr<RenderNode> > m_nodes;

    std::vector<RenderNode*> m_render_order;
    std::vector<RenderNode*> m_ordered;

public:
    template <typename T, typename... arg_ts>
    T &new_node(arg_ts&&... args)
    {
        T *node = new T(std::forward<arg_ts>(args)...);
        m_nodes.emplace_back(node);
        return *node;
    }

public:
    /**
     * Re-sort the nodes for rendering. This must be called when the
     * dependencies have been changed.
     *
     * This applies topological sort given the dependencies declared by the
     * nodes. If any cycles are found in the dependency graph, the sorting
     * fails and the list of nodes to render is cleared (but the nodes are kept
     * alive).
     *
     * @return true if the sorting succeeded, false if cycles are in the tree.
     * When this function returns false, rendering will not work.
     */
    bool resort();

    void render();
    void sync();

};


}

#endif
