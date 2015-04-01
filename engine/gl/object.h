#ifndef SCC_ENGINE_GL_OBJECT_H
#define SCC_ENGINE_GL_OBJECT_H

#include <GL/glew.h>

#include <functional>
#include <iostream>

#include "common/resource.h"


namespace engine {

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
