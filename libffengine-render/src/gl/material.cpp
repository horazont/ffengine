/**********************************************************************
File name: material.cpp
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
#include "ffengine/gl/material.hpp"

#include "ffengine/gl/util.hpp"

#include "ffengine/render/rendergraph.hpp"


namespace engine {

io::Logger &logger = io::logging().get_logger("gl.material");

Material::Material(const VBOFormat &format):
    m_max_texture_units(gl_get_integer(GL_MAX_TEXTURE_IMAGE_UNITS)),
    m_vbo(new VBO(format)),
    m_ibo(new IBO()),
    m_buffers_shared(false),
    m_base_free_unit(0)
{

}

Material::Material(VBO &vbo, IBO &ibo):
    m_max_texture_units(gl_get_integer(GL_MAX_TEXTURE_IMAGE_UNITS)),
    m_vbo(&vbo),
    m_ibo(&ibo),
    m_buffers_shared(true),
    m_base_free_unit(0)
{

}

Material::~Material()
{
    if (!m_buffers_shared) {
        if (m_vbo) {
            delete m_vbo;
        }
        if (m_ibo) {
            delete m_ibo;
        }
    }
}

GLint Material::get_next_texture_unit()
{
    if (m_free_units.size() > 0) {
        GLint result = m_free_units.back();
        m_free_units.erase(m_free_units.end()-1);
        return result;
    }

    if (m_base_free_unit >= m_max_texture_units) {
        throw std::runtime_error("out of texture units (max=" + std::to_string(m_max_texture_units)+")");
    }

    return m_base_free_unit++;
}

GLint Material::attach_texture(const std::string &name, Texture2D *tex)
{
    auto iter = m_texture_bindings.find(name);
    if (iter != m_texture_bindings.end()) {
        throw std::runtime_error("texture name already bound: "+name);
    }

    GLint unit = get_next_texture_unit();
    logger.logf(io::LOG_DEBUG, "binding %p: with name '%s'' at unit %d",
                tex, name.c_str(), unit);
    m_texture_bindings.emplace(
                name,
                TextureAttachment{name, unit, tex});


    if (m_shader.uniform_location(name) >= 0) {
        const ShaderUniform &uniform_info = m_shader.uniform(name);
        if (uniform_info.type != tex->shader_uniform_type()) {
            m_texture_bindings.erase(m_texture_bindings.find(name));
            throw std::runtime_error("incompatible types (uniform vs. texture)");
        }

        m_shader.bind();
        engine::raise_last_gl_error();
        logger.logf(io::LOG_DEBUG,
                    "assigning unit %d to sampler at location %d",
                    unit, uniform_info.loc);
        glUniform1i(uniform_info.loc, unit);
        engine::raise_last_gl_error();
    } else {
        logger.logf(io::LOG_DEBUG, "could not detect uniform location "
                                   "(may be inactive)");
    }

    return unit;
}

void Material::declare_attribute(const std::string &name, unsigned int vbo_attr, bool normalized)
{
    m_declaration.declare_attribute(name, *m_vbo, vbo_attr, normalized);
}

void Material::detach_texture(const std::string &name)
{
    auto iter = m_texture_bindings.find(name);
    if (iter == m_texture_bindings.end()) {
        return;
    }

    m_texture_bindings.erase(iter);
}

bool Material::link()
{
    if (!m_shader.link()) {
        logger.logf(io::LOG_DEBUG, "shader failed to link");
        return false;
    }

    logger.logf(io::LOG_DEBUG, "shader linked: %d", m_shader.glid());

    m_declaration.set_ibo(m_ibo);
    m_vao = std::move(m_declaration.make_vao(m_shader, true));
    RenderContext::configure_shader(m_shader);
    return true;
}

void Material::bind()
{
    m_vao->bind();
    m_shader.bind();
    for (auto &binding: m_texture_bindings)
    {
        glActiveTexture(GL_TEXTURE0+binding.second.texture_unit);
        binding.second.texture_obj->bind();
    }
}

void Material::sync()
{
    if (!m_buffers_shared) {
        m_ibo->sync();
        m_vbo->sync();
    }
}

}
