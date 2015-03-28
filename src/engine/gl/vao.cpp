#include "vao.h"

#include <algorithm>

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
    if (m_ibo_hint) {
        m_ibo_hint->bound();
    }
    for (auto &vbo: m_vbo_hints) {
        vbo->bind();
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
        for (auto &decl: m_attribs)
        {
            GLint loc = for_shader.attrib_location(decl.first);
            if (loc < 0) {
                throw std::runtime_error("shader does not have attribute: "+
                                         decl.first);
            }

            const VBOFinalAttribute &attr = decl.second.vbo.attrs().at(
                        decl.second.attr_index);
            decl.second.vbo.bind();
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(
                        loc,
                        attr.length,
                        GL_FLOAT,
                        (decl.second.normalized ? GL_TRUE : GL_FALSE),
                        decl.second.vbo.vertex_size(),
                        reinterpret_cast<GLvoid*>(attr.offset));

            if (add_vbo_hints) {
                result->add_vbo_hint(&decl.second.vbo);
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
