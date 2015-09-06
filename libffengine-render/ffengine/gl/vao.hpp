/**********************************************************************
File name: vao.hpp
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
#ifndef SCC_ENGINE_GL_VAO_H
#define SCC_ENGINE_GL_VAO_H

#include "ffengine/gl/object.hpp"
#include "ffengine/gl/vbo.hpp"
#include "ffengine/gl/ibo.hpp"
#include "ffengine/gl/shader.hpp"


namespace ffe {

class VAO: public GLObject<GL_VERTEX_ARRAY_BINDING>
{
public:
    VAO();

private:
    IBO *m_ibo_hint;
    std::vector<VBO*> m_vbo_hints;

protected:
    void delete_globject() override;

public:
    void add_vbo_hint(VBO *vbo_hint);
    void set_ibo_hint(IBO *ibo_hint);

public:
    void bind() override;
    void bound() override;
    void sync() override;
    void unbind() override;

};

struct AttributeMapping
{
    VBO &vbo;
    unsigned int attr_index;
    bool normalized;
};

class ArrayDeclaration
{
public:
    ArrayDeclaration();

private:
    IBO *m_ibo;
    std::unordered_map<std::string, AttributeMapping> m_attribs;

public:
    void declare_attribute(const std::string &name,
                           VBO &vbo,
                           unsigned int vbo_attr,
                           bool normalized=false);

    const AttributeMapping &get_attribute(const std::string &name) const;

    inline IBO *ibo() const
    {
        return m_ibo;
    }

    std::unique_ptr<VAO> make_vao(const ShaderProgram &for_shader,
                                  bool add_vbo_hints=false);

    void set_ibo(IBO *ibo);

};

}

#endif
