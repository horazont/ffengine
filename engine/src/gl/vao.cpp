/**********************************************************************
File name: vao.cpp
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
#include "engine/gl/vao.hpp"

#include <algorithm>

#include "engine/io/log.hpp"


namespace engine {

io::Logger &gl_vao_logger = io::logging().get_logger("gl.vao");


VAO::VAO():
    GLObject<GL_VERTEX_ARRAY_BINDING>(),
    m_ibo_hint(nullptr),
    m_vbo_hints()
{
    glGenVertexArrays(1, &m_glid);
}

void VAO::delete_globject()
{
    glDeleteVertexArrays(1, &m_glid);
}

void VAO::add_vbo_hint(VBO *vbo_hint)
{
    if (std::find(m_vbo_hints.begin(), m_vbo_hints.end(), vbo_hint) !=
            m_vbo_hints.end())
    {
        return;
    }
    m_vbo_hints.push_back(vbo_hint);
}

void VAO::set_ibo_hint(IBO *ibo_hint)
{
    m_ibo_hint = ibo_hint;
}

void VAO::bind()
{
    glBindVertexArray(m_glid);
    bound();
}

void VAO::bound()
{

}

void VAO::sync()
{
    bind();
    gl_vao_logger.logf(io::LOG_DEBUG, "synchronizing VAO (glid=%d)", m_glid);
    if (m_ibo_hint) {
        gl_vao_logger.logf(
                    io::LOG_DEBUG,
                    "synchronizing IBO hint (glid=%d)",
                    m_ibo_hint->glid());
        m_ibo_hint->sync();
    }
    for (auto &vbo: m_vbo_hints) {
        gl_vao_logger.logf(
                    io::LOG_DEBUG,
                    "synchronizing VBO hint (glid=%d)",
                    vbo->glid());
        vbo->sync();
    }
}

void VAO::unbind()
{
    glBindVertexArray(0);
}


ArrayDeclaration::ArrayDeclaration():
    m_ibo(0),
    m_attribs()
{

}

void ArrayDeclaration::declare_attribute(const std::string &name,
                                         VBO &vbo,
                                         unsigned int vbo_attr,
                                         bool normalized)
{
    {
        auto iter = m_attribs.find(name);
        if (iter != m_attribs.end())
        {
            throw std::runtime_error("attribute already declared: "+ name);
        }
    }

    m_attribs.emplace(
                name,
                AttributeMapping{vbo, vbo_attr, normalized}
                );
}

std::unique_ptr<VAO> ArrayDeclaration::make_vao(
        const ShaderProgram &for_shader,
        bool add_vbo_hints)
{
    std::unique_ptr<VAO> result = std::unique_ptr<VAO>(new VAO());
    result->bind();
    if (m_ibo) {
        m_ibo->bind();
        result->set_ibo_hint(m_ibo);
    }
    try {
        for (auto &attr: for_shader.attributes())
        {
            auto iter = m_attribs.find(attr.name);
            if (iter == m_attribs.end()) {
                throw std::runtime_error("No binding for vertex shader input "+attr.name);
            }
            const AttributeMapping &decl = iter->second;

            const VBOFinalAttribute &vbo_attr = decl.vbo.attrs().at(
                        decl.attr_index);
            decl.vbo.bind();
            glEnableVertexAttribArray(attr.loc);
            glVertexAttribPointer(
                        attr.loc,
                        vbo_attr.length,
                        GL_FLOAT,
                        (decl.normalized ? GL_TRUE : GL_FALSE),
                        decl.vbo.vertex_size(),
                        reinterpret_cast<GLvoid*>(vbo_attr.offset));

            if (add_vbo_hints) {
                result->add_vbo_hint(&decl.vbo);
            }
        }

    } catch (...) {
        if (m_ibo) {
            m_ibo->unbind();
        }
        throw;
    }

    result->unbind();
    return result;
}

void ArrayDeclaration::set_ibo(IBO *ibo)
{
    m_ibo = ibo;
}

}
