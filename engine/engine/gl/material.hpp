/**********************************************************************
File name: material.hpp
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
#ifndef SCC_ENGINE_GL_MATERIAL_H
#define SCC_ENGINE_GL_MATERIAL_H

#include <cassert>
#include <unordered_map>

#include "engine/gl/shader.hpp"
#include "engine/gl/texture.hpp"


namespace engine {

class Material: public Resource
{
public:
    struct TextureAttachment
    {
        std::string name;
        GLint texture_unit;
        _GLObjectBase *texture_obj;
    };

public:
    Material();

private:
    const GLint m_max_texture_units;
    ShaderProgram m_shader;
    std::unordered_map<std::string, TextureAttachment> m_texture_bindings;
    std::vector<GLint> m_free_units;
    GLint m_base_free_unit;

protected:
    GLint get_next_texture_unit();

public:
    inline ShaderProgram &shader()
    {
        return m_shader;
    }

public:
    GLint attach_texture(const std::string &name, Texture2D *tex);
    void detach_texture(const std::string &name);

public:
    void bind();

};

}

#endif
