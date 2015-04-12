#ifndef SCC_ENGINE_RENDERGRAPH_H
#define SCC_ENGINE_RENDERGRAPH_H

#include <vector>

#include "engine/gl/vao.hpp"
#include "engine/gl/material.hpp"
#include "engine/gl/fbo.hpp"
#include "engine/math/vector.hpp"


namespace engine {

typedef Vector<unsigned int, 2> ViewportSize;


class SceneGraph;
class Camera;


class SceneStorage
{
public:
    virtual ~SceneStorage();

};


class Scene
{
public:
    Scene(SceneGraph &scenegraph, Camera &camera);

private:
    SceneGraph &m_scenegraph;
    Camera &m_camera;

    Vector3f m_render_viewpoint;
    Matrix4f m_render_view;

    std::unordered_map<void*, std::unique_ptr<SceneStorage> > m_storage;

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

        return *iter->second;
    }

public:
    void set_render_view(const Matrix4f &view);
    void set_render_viewpoint(const Vector3f &viewpoint);

public:
    void sync();

};

/**
 * Track the environment in which a render takes place.
 */
class RenderContext
{
public:
    static constexpr GLint MATRIX_BLOCK_UBO_SLOT = 0;
    typedef UBO<Matrix4f, Matrix4f, Matrix4f, Matrix3f> MatrixUBO;

public:
    RenderContext(Scene &scene);

private:
    Scene &m_scene;
    GLsizei m_viewport_width, m_viewport_height;

    MatrixUBO m_matrix_ubo;
    std::vector<Matrix4f> m_model_stack;
    Matrix4f m_current_transformation;

    Vector3f m_viewpoint;

protected:
    void prepare_draw();

public:
    void draw_elements(GLenum primitive,
                       VAO &with_arrays, Material &using_material,
                       IBOAllocation &indicies);
    void draw_elements_base_vertex(GLenum primitive,
                                   VAO &with_arrays, Material &using_material,
                                   IBOAllocation &indicies,
                                   GLint base_vertex);

    void pop_transformation();

    void push_transformation(const Matrix4f &mat);

    void reset();

public:
    inline Scene &scene()
    {
        return m_scene;
    }

public:
    void set_viewport_size(GLsizei viewport_width, GLsizei viewport_height);

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


class RenderNode
{
public:
    typedef std::vector<RenderNode*> container_type;

public:
    RenderNode(RenderTarget &target);
    virtual ~RenderNode();

protected:
    RenderTarget &m_target;
    std::vector<RenderNode*> m_dependencies;

public:
    inline container_type &dependencies()
    {
        return m_dependencies;
    }

    inline const container_type &dependencies() const
    {
        return m_dependencies;
    }

public:
    virtual void render() = 0;
    virtual void sync() = 0;

};


class BlitNode: public RenderNode
{
public:
    BlitNode(RenderTarget &src, RenderTarget &dest);

protected:
    RenderTarget &m_src;

public:
    void render() override;
    void sync() override;

};


class SceneRenderNode: public RenderNode
{
public:
    SceneRenderNode(RenderTarget &target, Scene &scene);

protected:
    Scene &m_scene;
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
    void set_clear_mask(GLbitfield mask);
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


class RenderGraph
{
public:
    RenderGraph();

private:
    std::vector<std::unique_ptr<Scene> > m_locked_scenes;
    std::vector<std::unique_ptr<Scene> > m_scenes;

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

    Scene &new_scene(SceneGraph &scenegraph, Camera &camera);

public:
    bool resort();
    void render();
    void sync();

};


}

#endif
