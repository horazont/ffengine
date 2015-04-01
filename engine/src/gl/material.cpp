#include "engine/gl/material.hpp"

#include "engine/gl/util.hpp"


namespace engine {

Material::Material():
    m_max_texture_units(gl_get_integer(GL_MAX_TEXTURE_UNITS)),
    m_base_free_unit(0)
{

}

GLint Material::get_next_texture_unit()
{
    if (m_free_units.size() > 0) {
        GLint result = m_free_units.back();
        m_free_units.erase(m_free_units.end()-1);
        return result;
    }

    if (m_base_free_unit >= m_max_texture_units) {
        throw std::runtime_error("out of texture units");
    }

    return m_base_free_unit++;
}

GLint Material::attach_texture(const std::string &name, Texture *tex)
{
    auto iter = m_texture_bindings.find(name);
    if (iter != m_texture_bindings.end()) {
        throw std::runtime_error("texture name already bound: "+name);
    }

    const ShaderUniform &uniform_info = m_shader.uniform(name);
    if (uniform_info.type != tex->shader_uniform_type()) {
        throw std::runtime_error("incompatible types (uniform vs. texture)");
    }

    GLint unit = get_next_texture_unit();
    m_texture_bindings.emplace(
                name,
                TextureAttachment{name, unit, tex});

    m_shader.bind();
    glUniform1i(uniform_info.loc, unit);
    return unit;
}

void Material::detach_texture(const std::string &name)
{
    auto iter = m_texture_bindings.find(name);
    if (iter == m_texture_bindings.end()) {
        return;
    }

    m_texture_bindings.erase(iter);
}

}
