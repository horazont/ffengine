/**********************************************************************
File name: skycube.cpp
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
#include "ffengine/render/skycube.hpp"

#include "ffengine/gl/texture.hpp"
#include "ffengine/gl/resource.hpp"


namespace ffe {

SkyCubeNode::SkyCubeNode(GLResourceManager &resources,
                         RenderPass &target_pass):
    m_mat(VBOFormat({VBOAttribute(3)})),
    m_vbo_alloc(m_mat.vbo().allocate(8)),
    m_ibo_alloc(m_mat.ibo().allocate(3*2*6))
{
    {
        spp::EvaluationContext ctx(resources.shader_library());
        MaterialPass &pass = m_mat.make_pass_material(target_pass);

        bool success = true;
        success = success && pass.shader().attach(
                    resources.load_shader_checked(":/shaders/skycube/skycube.vert"),
                    ctx,
                    GL_VERTEX_SHADER);
        success = success && pass.shader().attach(
                    resources.load_shader_checked(":/shaders/skycube/skycube.frag"),
                    ctx,
                    GL_FRAGMENT_SHADER);

        m_mat.declare_attribute("position", 0);

        success = success && m_mat.link();

        if (!success) {
            throw std::runtime_error("failed to compile or link skycube material");
        }

        pass.set_order(-100);
        pass.set_depth_test(false);
        pass.set_depth_mask(false);
    }

    m_mat.attach_texture("skycube", resources.get_safe<TextureCubeMap>("skycube"));

    {
        auto slice = VBOSlice<Vector3f>(m_vbo_alloc, 0);
        slice[0] = Vector3f(-1.0f, -1.0f, -1.0f);
        slice[1] = Vector3f(1.0f, -1.0f, -1.0f);
        slice[2] = Vector3f(1.0f, 1.0f, -1.0f);
        slice[3] = Vector3f(-1.0f, 1.0f, -1.0f);

        slice[4] = Vector3f(-1.0f, -1.0f, 1.0f);
        slice[5] = Vector3f(-1.0f, 1.0f, 1.0f);
        slice[6] = Vector3f(1.0f, 1.0f, 1.0f);
        slice[7] = Vector3f(1.0f, -1.0f, 1.0f);
    }

    {
        uint16_t *dest = m_ibo_alloc.get();
        // bottom
        *dest++ = 1;
        *dest++ = 2;
        *dest++ = 0;

        *dest++ = 0;
        *dest++ = 2;
        *dest++ = 3;

        // back
        *dest++ = 0;
        *dest++ = 4;
        *dest++ = 1;

        *dest++ = 1;
        *dest++ = 4;
        *dest++ = 7;

        // right
        *dest++ = 2;
        *dest++ = 1;
        *dest++ = 6;

        *dest++ = 6;
        *dest++ = 1;
        *dest++ = 7;

        // front
        *dest++ = 3;
        *dest++ = 2;
        *dest++ = 5;

        *dest++ = 5;
        *dest++ = 2;
        *dest++ = 6;

        // left
        *dest++ = 4;
        *dest++ = 0;
        *dest++ = 5;

        *dest++ = 5;
        *dest++ = 0;
        *dest++ = 3;

        // top
        *dest++ = 4;
        *dest++ = 5;
        *dest++ = 7;

        *dest++ = 7;
        *dest++ = 5;
        *dest++ = 6;
    }

    m_vbo_alloc.mark_dirty();
    m_ibo_alloc.mark_dirty();
    m_mat.sync_buffers();
}

void SkyCubeNode::prepare(RenderContext &)
{

}

void SkyCubeNode::render(RenderContext &context)
{
    context.render_all(AABB{}, GL_TRIANGLES, m_mat, m_ibo_alloc, m_vbo_alloc,
                       nullptr,
                       nullptr);
}

void SkyCubeNode::sync()
{

}

}
