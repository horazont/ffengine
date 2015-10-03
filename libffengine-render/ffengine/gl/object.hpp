/**********************************************************************
File name: object.hpp
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
#ifndef SCC_ENGINE_GL_OBJECT_H
#define SCC_ENGINE_GL_OBJECT_H

#include <epoxy/gl.h>

#include <functional>
#include <iostream>

#include "ffengine/common/resource.hpp"


namespace ffe {

class _GLObjectBase: public Resource
{
protected:
    _GLObjectBase();

public:
    _GLObjectBase(_GLObjectBase &&ref);
    _GLObjectBase &operator=(_GLObjectBase &&ref);

public:
    virtual ~_GLObjectBase();

protected:
    GLuint m_glid;

protected:
    virtual void delete_globject();

public:
    inline GLuint glid() const
    {
        return m_glid;
    }

public:
    virtual void bind() = 0;
    virtual void bound();
    virtual void sync() = 0;
    virtual void unbind() = 0;

};

template <GLuint _binding_type>
class GLObject: public _GLObjectBase
{
public:
    static constexpr GLuint binding_type = _binding_type;

public:
    bool is_bound()
    {
        GLuint binding;
        glGetIntegerv(binding_type, reinterpret_cast<GLint*>(&binding));
        return binding == m_glid;
    }

};

}

#endif
