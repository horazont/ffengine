/**********************************************************************
File name: renderpass.cpp
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
#include "ffengine/render/renderpass.hpp"

namespace ffe {

static io::Logger &logger = io::logging().get_logger("renderpass");

/* ffe::Scene */

Scene::Scene(SceneGraph &scenegraph, Camera &camera):
    m_scenegraph(scenegraph),
    m_camera(camera)
{

}

/* ffe::MaterialPass */

MaterialPass::MaterialPass(Material &material, RenderPass &pass):
    m_material(material),
    m_pass(pass),
    m_order(0),
    m_base_free_unit(0)
{

}

GLint MaterialPass::get_next_texture_unit()
{
    if (m_free_units.size() > 0) {
        GLint result = m_free_units.back();
        m_free_units.pop_back();
        return result;
    }

    return m_base_free_unit++;
}

bool MaterialPass::link()
{
    if (is_linked()) {
        return true;
    }

    if (!m_shader.link()) {
        logger.logf(io::LOG_DEBUG, "shader failed to link");
        return false;
    }

    m_vao = std::move(m_material.vertex_attrs().make_vao(m_shader, true));
    RenderContext::configure_shader(m_shader);
    return true;
}

bool MaterialPass::attach_texture(const std::string &name, Texture2D *tex)
{
    if (!is_linked()) {
        throw std::logic_error("cannot attach texture to unlinked material");
    }

    {
        auto iter = m_texture_bindings.find(name);
        if (iter != m_texture_bindings.end()) {
            throw std::runtime_error("texture name already bound: "+name);
        }
    }

    GLint unit = get_next_texture_unit();
    logger.logf(io::LOG_DEBUG, "binding %p to name `%s' at unit %d",
                tex, name.c_str(), unit);

    if (m_shader.uniform_location(name) >= 0) {
        const ShaderUniform &uniform_info = m_shader.uniform(name);
        if (uniform_info.type != tex->shader_uniform_type()) {
            return false;
        }

        m_shader.bind();
        ffe::raise_last_gl_error();
        logger.logf(io::LOG_DEBUG,
                    "assigning unit %d to sampler at location %d",
                    unit, uniform_info.loc);
        glUniform1i(uniform_info.loc, unit);
        ffe::raise_last_gl_error();
    } else {
        logger.logf(io::LOG_WARNING, "texture uniform `%s' may be inactive",
                    name.c_str());
        return false;
    }

    m_texture_bindings.emplace(name, TextureAttachment{name, unit, tex});

    return true;
}

void MaterialPass::bind()
{
    assert(is_linked());
    m_vao->bind();
    m_shader.bind();
    for (auto &binding: m_texture_bindings) {
        glActiveTexture(GL_TEXTURE0 + binding.second.texture_unit);
        binding.second.texture_obj->bind();
    }
}

void MaterialPass::detach_texture(const std::string &name)
{
    auto iter = m_texture_bindings.find(name);
    if (iter == m_texture_bindings.end()) {
        return;
    }

    m_free_units.push_back(iter->second.texture_unit);

    m_texture_bindings.erase(iter);
}

void MaterialPass::set_order(int order)
{
    m_order = order;
}

void MaterialPass::setup()
{
    bind();
    m_material.setup();
}

void MaterialPass::teardown()
{
    m_material.teardown();
}

/* ffe::Material */

Material::Material():
    m_buffers_owned(false),
    m_vbo(nullptr),
    m_ibo(nullptr),
    m_linked(false),
    m_polygon_mode(GL_FILL),
    m_depth_mask(true)
{

}

Material::Material(const VBOFormat &format):
    m_buffers_owned(true),
    m_vbo(new VBO(format)),
    m_ibo(new IBO),
    m_linked(false),
    m_polygon_mode(GL_FILL),
    m_depth_mask(true)
{
    m_vertex_attrs.set_ibo(m_ibo);
}

Material::Material(VBO &vbo, IBO &ibo):
    m_buffers_owned(false),
    m_vbo(&vbo),
    m_ibo(&ibo),
    m_linked(false),
    m_polygon_mode(GL_FILL),
    m_depth_mask(true)
{
    m_vertex_attrs.set_ibo(m_ibo);
}

Material::Material(Material &&src):
    m_buffers_owned(src.m_buffers_owned),
    m_vbo(std::move(src.m_vbo)),
    m_ibo(std::move(src.m_ibo)),
    m_linked(src.m_linked),
    m_polygon_mode(src.m_polygon_mode),
    m_depth_mask(src.m_depth_mask),
    m_vertex_attrs(std::move(src.m_vertex_attrs)),
    m_passes(std::move(src.m_passes))
{
    src.m_buffers_owned = false;
    src.m_vbo = nullptr;
    src.m_ibo = nullptr;
}

Material &Material::operator=(Material &&src)
{
    if (m_buffers_owned) {
        delete m_ibo;
        delete m_vbo;
    }
    m_buffers_owned = src.m_buffers_owned;
    m_vbo = src.m_vbo;
    src.m_vbo = nullptr;
    m_ibo = src.m_ibo;
    src.m_ibo = nullptr;
    m_linked = src.m_linked;
    src.m_linked = false;
    m_polygon_mode = src.m_polygon_mode;
    m_depth_mask = src.m_depth_mask;
    m_vertex_attrs = std::move(src.m_vertex_attrs);
    m_passes = std::move(src.m_passes);
    return *this;
}

Material::~Material()
{
    if (m_buffers_owned) {
        delete m_vbo;
        delete m_ibo;
    }
}

void Material::setup()
{
    if (m_polygon_mode != GL_FILL) {
        glPolygonMode(GL_FRONT_AND_BACK, m_polygon_mode);
    }
    if (!m_depth_mask) {
        glDepthMask(GL_FALSE);
    }
}

void Material::teardown()
{
    if (!m_depth_mask) {
        glDepthMask(GL_TRUE);
    }
    if (m_polygon_mode != GL_FILL) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void Material::attach_texture(const std::string &name, Texture2D *tex)
{
    for (auto &pass_entry: m_passes) {
        pass_entry.second->attach_texture(name, tex);
    }
}

void Material::declare_attribute(const std::string &name,
                                 unsigned int nattr,
                                 bool normalized)
{
    if (is_linked()) {
        throw std::logic_error("cannot declare attribute after material has "
                               "been linked");
    }

    m_vertex_attrs.declare_attribute(name, *m_vbo, nattr, normalized);
}

bool Material::link()
{
    if (is_linked()) {
        return true;
    }

    for (auto &entry: m_passes) {
        if (!entry.second->link()) {
            throw std::runtime_error("failed to link pass material");
        }
    }

    m_linked = true;
    return true;
}

MaterialPass &Material::make_pass_material(RenderPass &pass)
{
    {
        auto iter = m_passes.find(&pass);
        if (iter != m_passes.end()) {
            return *iter->second;
        }
    }

    auto pass_material(std::make_unique<MaterialPass>(*this, pass));
    MaterialPass &result = *pass_material;
    m_passes[&pass] = std::move(pass_material);

    return result;
}

MaterialPass *Material::pass_material(RenderPass &pass)
{
    auto iter = m_passes.find(&pass);
    if (iter == m_passes.end()) {
        return nullptr;
    }

    return iter->second.get();
}

void Material::sync_buffers()
{
    m_vbo->sync();
    m_ibo->sync();
}

/* ffe::PassRenderInstruction */

PassRenderInstruction::PassRenderInstruction(
        const AABB &box,
        GLint mode,
        MaterialPass &mat,
        IBOAllocation &ibo_allocation,
        VBOAllocation &vbo_allocation,
        const RenderSetupFunc &setup,
        const RenderTeardownFunc &teardown):
    box(box),
    mode(mode),
    material_pass(&mat),
    ibo_allocation(&ibo_allocation),
    vbo_allocation(&vbo_allocation),
    setup(setup),
    teardown(teardown)
{

}

/* ffe::PassInfo */

void PassInfo::emplace_instruction(const AABB &box,
                                   GLint mode,
                                   MaterialPass &mat,
                                   IBOAllocation &ibo_allocation,
                                   VBOAllocation &vbo_allocation,
                                   const RenderSetupFunc &setup,
                                   const RenderTeardownFunc &teardown)
{
    m_instructions.emplace_back(box, mode, mat,
                                ibo_allocation, vbo_allocation,
                                setup, teardown);
}

void PassInfo::render()
{
    MaterialPass *prev = nullptr;
    for (PassRenderInstruction &instruction: m_instructions) {
        MaterialPass *curr = instruction.material_pass;
        if (curr != prev) {
            if (prev) {
                prev->teardown();
            }
            curr->setup();
            prev = curr;
        }
        if (instruction.setup) {
            instruction.setup(*curr);
        }
        draw_elements_base_vertex(*instruction.ibo_allocation,
                                  instruction.mode,
                                  instruction.vbo_allocation->base());
        if (instruction.teardown) {
            instruction.teardown(*curr);
        }
    }
    if (prev) {
        prev->teardown();
    }
    reset();
}

void PassInfo::reset()
{
    m_instructions.clear();
}

void PassInfo::sort_instructions()
{
    // TODO: implement this
}

/* ffe::RenderNode */

RenderNode::RenderNode(RenderTarget &target):
    m_target(target)
{

}

RenderNode::~RenderNode()
{

}

/* ffe::RenderPass */

RenderPass::RenderPass(RenderTarget &target):
    RenderNode(target),
    m_blit_colour_src(nullptr),
    m_blit_depth_src(nullptr),
    m_clear_mask(0)
{

}

void RenderPass::set_blit_colour_src(RenderTarget *src)
{
    m_blit_colour_src = src;
}

void RenderPass::set_blit_depth_src(RenderTarget *src)
{
    m_blit_depth_src = src;
}

void RenderPass::set_clear_mask(GLbitfield mask)
{
    m_clear_mask = mask;
}

void RenderPass::set_clear_colour(const Vector4f &colour)
{
    m_clear_colour = colour;
}

void RenderPass::render(RenderContext &context)
{
    m_target.bind(RenderTarget::Usage::DRAW);
    if (m_clear_mask) {
        glClearColor(m_clear_colour[eX], m_clear_colour[eY],
                     m_clear_colour[eZ], m_clear_colour[eW]);
        glClear(m_clear_mask);
    }

    if (m_blit_colour_src) {
        GLbitfield blit = 0;
        m_blit_colour_src->bind(RenderTarget::Usage::READ);
        if (m_blit_depth_src == m_blit_colour_src) {
            blit |= GL_DEPTH_BUFFER_BIT;
        }
        blit |= GL_COLOR_BUFFER_BIT;
        glBlitFramebuffer(0, 0, m_blit_colour_src->width(), m_blit_colour_src->height(),
                          0, 0, m_target.width(), m_target.height(),
                          blit,
                          GL_NEAREST);
    }

    if (m_blit_depth_src && m_blit_depth_src != m_blit_colour_src) {
        m_blit_depth_src->bind(RenderTarget::Usage::READ);
        glBlitFramebuffer(0, 0, m_blit_depth_src->width(), m_blit_depth_src->height(),
                          0, 0, m_target.width(), m_target.height(),
                          GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
    }

    PassInfo &info = context.pass_info(this);
    info.sort_instructions();
    info.render();
}

/* ffe::RenderContext */

void RenderContext::render_all(const AABB &box,
                               GLint mode,
                               Material &material,
                               IBOAllocation &indices,
                               VBOAllocation &vertices,
                               RenderSetupFunc setup,
                               RenderTeardownFunc teardown)
{
    for (auto iter = material.cbegin();
         iter != material.cend();
         ++iter)
    {
        render_pass(box, mode, *iter->second, indices, vertices, setup, teardown);
    }
}

void RenderContext::render_pass(const AABB &box,
                                GLint mode,
                                MaterialPass &material_pass,
                                IBOAllocation &indices,
                                VBOAllocation &vertices,
                                RenderSetupFunc setup,
                                RenderTeardownFunc teardown)
{
    PassInfo &info = pass_info(&material_pass.pass());
    info.emplace_instruction(box, mode, material_pass,
                             indices, vertices,
                             setup, teardown);
}

PassInfo &RenderContext::pass_info(RenderPass *pass)
{
    return m_passes[pass];
}

void RenderContext::setup(const Camera &camera,
                          const SceneGraph &scenegraph,
                          const RenderTarget &target)
{
    Matrix4f render_view = camera.render_view();
    Matrix4f inv_render_view = camera.render_inv_view();

    m_viewpoint = Vector3f(inv_render_view * Vector4f(0, 0, 0, 1));

    std::tie(m_matrix_ubo.get_ref<0>(),
             m_inv_matrix_ubo.get_ref<0>()) = camera.render_projection(
                target.width(), target.height());

    m_matrix_ubo.set<1>(render_view);
    m_matrix_ubo.set<2>(scenegraph.sun_colour());
    m_matrix_ubo.set<3>(scenegraph.sun_direction());
    m_matrix_ubo.set<4>(scenegraph.sky_colour());
    m_matrix_ubo.set<5>(m_viewpoint);
    m_inv_matrix_ubo.set<1>(inv_render_view);
    m_inv_matrix_ubo.set<2>(Vector2f(target.width(), target.height()));
    m_inv_matrix_ubo.bind();
    m_inv_matrix_ubo.update_bound();

    m_matrix_ubo.bind();
    m_matrix_ubo.update_bound();

    Matrix4f projview = (m_matrix_ubo.get_ref<0>() * render_view).transposed();

    m_frustum[0] = Plane::from_frustum_matrix(projview * Vector4f(1, 0, 0, 1));
    m_frustum[1] = Plane::from_frustum_matrix(projview * Vector4f(-1, 0, 0, 1));
    m_frustum[2] = Plane::from_frustum_matrix(projview * Vector4f(0, 1, 0, 1));
    m_frustum[3] = Plane::from_frustum_matrix(projview * Vector4f(0, -1, 0, 1));
    m_frustum[4] = Plane::from_frustum_matrix(projview * Vector4f(0, 0, 1, 1));
    m_frustum[5] = Plane::from_frustum_matrix(projview * Vector4f(0, 0, -1, 1));
}

void RenderContext::start_render()
{
    m_inv_matrix_ubo.bind_at(INV_MATRIX_BLOCK_UBO_SLOT);
    m_matrix_ubo.bind();
    m_matrix_ubo.bind_at(MATRIX_BLOCK_UBO_SLOT);
}

void RenderContext::configure_shader(ShaderProgram &shader)
{
    if (shader.uniform_block_location("MatrixBlock") >= 0) {
        shader.check_uniform_block<MatrixUBO>("MatrixBlock");
        shader.bind_uniform_block("MatrixBlock", MATRIX_BLOCK_UBO_SLOT);
    }
    if (shader.uniform_block_location("InvMatrixBlock") >= 0) {
        shader.check_uniform_block<InvMatrixUBO>("InvMatrixBlock");
        shader.bind_uniform_block("InvMatrixBlock", INV_MATRIX_BLOCK_UBO_SLOT);
    }
}

/* ffe::RenderGraph */

RenderGraph::RenderGraph(Scene &scene):
    m_scene(scene)
{

}

bool RenderGraph::resort()
{
    m_ordered.clear();

    std::vector<RenderNode*> no_incoming_edges;
    std::unordered_map<RenderNode*, std::vector<RenderNode*> > tmp_deps;
    no_incoming_edges.reserve(m_nodes.size());

    for (auto &node_ptr: m_nodes) {
        RenderNode &node = *node_ptr;
        if (node.dependencies().empty()) {
            no_incoming_edges.push_back(&node);
        } else {
            std::vector<RenderNode*> &vector = tmp_deps[&node];
            vector.reserve(node.dependencies().size());
            std::copy(node.dependencies().begin(),
                      node.dependencies().end(),
                      std::back_inserter(vector));
        }
    }

    while (!no_incoming_edges.empty()) {
        RenderNode *node = no_incoming_edges.back();
        m_ordered.push_back(node);
        no_incoming_edges.pop_back();

        auto iter = tmp_deps.begin();
        while (iter != tmp_deps.end())
        {
            auto depiter = std::find(iter->second.begin(),
                                     iter->second.end(),
                                     node);
            if (depiter == iter->second.end()) {
                ++iter;
                continue;
            }

            iter->second.erase(depiter);
            if (iter->second.empty()) {
                no_incoming_edges.push_back(iter->first);
                iter = tmp_deps.erase(iter);
                continue;
            }

            ++iter;
        }
    }

    if (tmp_deps.size() > 0) {
        m_ordered.clear();
        logger.logf(io::LOG_ERROR, "render graph has cycles; won’t render this shit");
        return false;
    }

    return true;
}

void RenderGraph::render()
{
    m_context.start_render();
    m_scene.scenegraph().render(m_context);
    for (RenderNode *node: m_render_order)
    {
        node->render(m_context);
    }
}

void RenderGraph::prepare()
{
    m_locked_nodes.clear();
    m_render_order = m_ordered;

    // we use the final target as reference for the viewport
    // if we ever need distinct targets sizes, we need to figure out how to
    // solve that best.
    m_context.setup(m_scene.camera(), m_scene.scenegraph(),
                    m_render_order.back()->target());
    m_scene.scenegraph().prepare(m_context);
}

}

