/**********************************************************************
File name: renderpass.hpp
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
#ifndef SCC_RENDER_RENDERPASS_H
#define SCC_RENDER_RENDERPASS_H

#include <array>
#include <functional>
#include <unordered_map>

#include "ffengine/gl/fbo.hpp"
#include "ffengine/gl/texture.hpp"

#include "ffengine/render/scenegraph.hpp"


namespace ffe {

class RenderContext;

class Scene
{
public:
    Scene(SceneGraph &scenegraph, Camera &camera);

private:
    SceneGraph &m_scenegraph;
    Camera &m_camera;

public:
    inline Camera &camera()
    {
        return m_camera;
    }

    inline const Camera &camera() const
    {
        return m_camera;
    }

    inline SceneGraph &scenegraph()
    {
        return m_scenegraph;
    }

    inline const SceneGraph &scenegraph() const
    {
        return m_scenegraph;
    }

};


enum ZOrder
{
    ZORDER_UNSPECIFIED = 0,
    ZORDER_FRONT_TO_BACK = 1,
};


class Material;
class RenderPass;

class MaterialPass
{
private:
    struct TextureAttachment
    {
        std::string name;
        GLint texture_unit;
        Texture *texture_obj;
    };

public:
    MaterialPass(Material &material, RenderPass &pass);
    MaterialPass(const MaterialPass &ref) = delete;
    MaterialPass &operator=(const MaterialPass &ref) = delete;
    MaterialPass(MaterialPass &&src) = delete;
    MaterialPass &operator=(MaterialPass &&src) = delete;

private:
    Material &m_material;
    RenderPass &m_pass;
    int m_order;
    ffe::ShaderProgram m_shader;

    std::unordered_map<std::string, TextureAttachment> m_texture_bindings;
    std::vector<GLint> m_free_units;
    GLint m_base_free_unit;

    std::unique_ptr<ffe::VAO> m_vao;

private:
    GLint get_next_texture_unit();

protected:
    bool link();

public:
    inline bool is_linked() const
    {
        return bool(m_vao);
    }

    inline RenderPass &pass()
    {
        return m_pass;
    }

    inline ShaderProgram &shader()
    {
        return m_shader;
    }

    inline const ShaderProgram &shader() const
    {
        return m_shader;
    }

public:
    bool attach_texture(const std::string &name, Texture *tex);
    void bind();
    void detach_texture(const std::string &name);
    void set_order(int order);
    void setup();
    void teardown();

    friend class Material;
};


class Material: public Resource
{
public:
    typedef std::unordered_map<RenderPass*, std::unique_ptr<MaterialPass> > internal_container;
    typedef typename internal_container::const_iterator iterator;

public:
    Material();
    explicit Material(const VBOFormat &format);
    Material(VBO &vbo, IBO &ibo);

    Material(const Material &ref) = delete;
    Material &operator=(const Material &ref) = delete;

    Material(Material &&src);
    Material &operator=(Material &&src);

    ~Material() override;

private:
    bool m_buffers_owned;
    VBO *m_vbo;
    IBO *m_ibo;

    bool m_linked;

    GLenum m_polygon_mode;
    bool m_depth_mask;
    bool m_depth_test;
    float m_point_size;

    ArrayDeclaration m_vertex_attrs;

    std::unordered_map<RenderPass*, std::unique_ptr<MaterialPass> > m_passes;

protected:
    void setup();
    void teardown();

public:
    void attach_texture(const std::string &name, Texture *tex);

    inline iterator cbegin() const
    {
        return m_passes.cbegin();
    }

    inline iterator cend() const
    {
        return m_passes.cend();
    }

    void declare_attribute(const std::string &name,
                           unsigned int nattr,
                           bool normalized = false);

    inline IBO &ibo()
    {
        return *m_ibo;
    }

    inline const IBO &ibo() const
    {
        return *m_ibo;
    }

    inline bool is_linked() const
    {
        return m_linked;
    }

    bool link();

    MaterialPass &make_pass_material(RenderPass &pass);

    MaterialPass *pass_material(RenderPass &pass);

    void sync_buffers();

    inline VBO &vbo()
    {
        return *m_vbo;
    }

    inline const VBO &vbo() const
    {
        return *m_vbo;
    }

    inline const ArrayDeclaration &vertex_attrs() const
    {
        return m_vertex_attrs;
    }

    inline operator bool() const
    {
        return (m_ibo && m_vbo);
    }

public:
    inline bool depth_mask() const
    {
        return m_depth_mask;
    }

    inline bool depth_test() const
    {
        return m_depth_test;
    }

    inline GLenum polygon_mode() const
    {
        return m_polygon_mode;
    }

    inline float point_size() const
    {
        return m_point_size;
    }

    inline void set_depth_mask(const bool mask)
    {
        m_depth_mask = mask;
    }

    inline void set_depth_test(const bool enabled)
    {
        m_depth_test = enabled;
    }

    inline void set_polygon_mode(const GLenum mode)
    {
        m_polygon_mode = mode;
    }

    inline void set_point_size(const float size)
    {
        m_point_size = size;
    }

public:
    static inline std::unique_ptr<Material> shared_with(Material &ref)
    {
        return std::make_unique<Material>(ref.vbo(), ref.ibo());
    }

    friend class MaterialPass;
};


typedef std::function<void(MaterialPass&)> RenderSetupFunc;
typedef std::function<void(MaterialPass&)> RenderTeardownFunc;


struct PassRenderInstruction
{
    PassRenderInstruction(const AABB &box,
                          GLint mode,
                          MaterialPass &mat,
                          IBOAllocation &ibo_allocation,
                          VBOAllocation &vbo_allocation,
                          const RenderSetupFunc &setup,
                          const RenderTeardownFunc &teardown);
    PassRenderInstruction(PassRenderInstruction &&src) = default;
    PassRenderInstruction& operator=(PassRenderInstruction &&src) = default;

    AABB box;
    GLint mode;
    MaterialPass *material_pass;
    IBOAllocation *ibo_allocation;
    VBOAllocation *vbo_allocation;
    RenderSetupFunc setup;
    RenderTeardownFunc teardown;
};

class PassInfo
{
private:
    std::vector<PassRenderInstruction> m_instructions;

public:
    void emplace_instruction(const AABB &box,
                             GLint mode,
                             MaterialPass &mat,
                             IBOAllocation &ibo_allocation,
                             VBOAllocation &vbo_allocation,
                             const RenderSetupFunc &setup,
                             const RenderTeardownFunc &teardown);

    /**
     * Render all instructions as currently in the list.
     */
    void render();

    /**
     * Clear all stored data, but leave memory for storing data allocated.
     */
    void reset();

    /**
     * Sort the render instructions according to the settings specified in the
     * associated renderpass.
     *
     * If the renderpass does not specify a Z order, the objects are grouped
     * soley by material. If the renderpass specifies a Z order, the objects are
     * ordered by the nearest Z coordinate of their bounding volume.
     */
    void sort_instructions();

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

    inline RenderTarget &target()
    {
        return m_target;
    }

    inline const RenderTarget &target() const
    {
        return m_target;
    }

public:
    virtual void render(RenderContext &context) = 0;

};


/**
 * Render a SceneGraph with a Camera into the given target.
 */
class RenderPass: public RenderNode
{
public:
    RenderPass(RenderTarget &target);

protected:
    RenderTarget *m_blit_colour_src;
    RenderTarget *m_blit_depth_src;

    GLbitfield m_clear_mask;
    Vector4f m_clear_colour;

public:
    inline RenderTarget *blit_colour_src() const
    {
        return m_blit_colour_src;
    }

    inline RenderTarget *blit_depth_src() const
    {
        return m_blit_depth_src;
    }

    inline GLbitfield clear_mask() const
    {
        return m_clear_mask;
    }

    inline const Vector4f &clear_colour() const
    {
        return m_clear_colour;
    }

public:
    void set_blit_colour_src(RenderTarget *src);
    void set_blit_depth_src(RenderTarget *src);

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
    void render(RenderContext &context) override;

};


class RenderContext
{
public:
    static constexpr GLint MATRIX_BLOCK_UBO_SLOT = 0;
    typedef UBO<Matrix4f, Matrix4f, Vector4f, Vector3f, Vector4f, Vector3f> MatrixUBO;
    static constexpr GLint INV_MATRIX_BLOCK_UBO_SLOT = 1;
    typedef UBO<Matrix4f, Matrix4f, Vector2f> InvMatrixUBO;

private:
    std::unordered_map<RenderPass*, PassInfo> m_passes;
    MatrixUBO m_matrix_ubo;
    InvMatrixUBO m_inv_matrix_ubo;
    std::array<Plane, 6> m_frustum;
    Vector3f m_viewpoint;

public:
    inline const std::array<Plane, 6> &frustum() const
    {
        return m_frustum;
    }

    inline const Vector3f &viewpoint() const
    {
        return m_viewpoint;
    }

public:
    void render_all(const AABB &box,
                    GLint mode,
                    Material &material,
                    ffe::IBOAllocation &indices,
                    ffe::VBOAllocation &vertices,
                    RenderSetupFunc setup = nullptr,
                    RenderTeardownFunc teardown = nullptr);
    void render_pass(const AABB &box,
                     GLint mode,
                     MaterialPass &material_pass,
                     ffe::IBOAllocation &indices,
                     ffe::VBOAllocation &vertices,
                     RenderSetupFunc setup = nullptr,
                     RenderTeardownFunc teardown = nullptr);

public:
    PassInfo &pass_info(RenderPass *pass);
    void setup(const Camera &camera,
               const SceneGraph &scenegraph,
               const RenderTarget &target);
    void start_render();

public:
    static void configure_shader(ShaderProgram &shader);

};


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
    RenderGraph(Scene &scene);

private:
    Scene &m_scene;

    std::vector<std::unique_ptr<RenderNode> > m_locked_nodes;
    std::vector<std::unique_ptr<RenderNode> > m_nodes;

    std::vector<RenderNode*> m_render_order;
    std::vector<RenderNode*> m_ordered;

    RenderContext m_context;

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
    void prepare();

};


}

#endif
