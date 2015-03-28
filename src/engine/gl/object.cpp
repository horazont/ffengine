#include "object.h"

#include "GL/glu.h"

#include <string>
#include <stdexcept>

#include <cassert>

_GLObjectBase::_GLObjectBase():
    m_glid(0)
{

}

_GLObjectBase::~_GLObjectBase()
{
    if (m_glid != 0) {
        delete_globject();
    }
}

_GLObjectBase::_GLObjectBase(_GLObjectBase &&ref):
    m_glid(ref.m_glid)
{
    ref.m_glid = 0;
}

_GLObjectBase &_GLObjectBase::operator=(_GLObjectBase &&ref)
{
    if (m_glid != 0) {
        delete_globject();
    }
    m_glid = ref.m_glid;
    return *this;
}

void _GLObjectBase::delete_globject()
{
    m_glid = 0;
}

void _GLObjectBase::bound()
{

}


void raise_last_gl_error()
{
    const GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        throw std::runtime_error("OpenGL error: " +
                                 std::string((const char*)gluErrorString(err)));
    }
}
