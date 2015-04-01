#ifndef SCC_ENGINE_GL_TEXTURE_H
#define SCC_ENGINE_GL_TEXTURE_H

#include <GL/glew.h>

#include "engine/gl/object.hpp"
#include "engine/gl/2darray.hpp"


namespace engine {

class Texture2D: public GLObject<GL_TEXTURE_BINDING_2D>,
                 public GL2DArray
{
public:
    Texture2D(const GLenum internal_format,
              const GLsizei width,
              const GLsizei height);

public:
    void bind() override;
    void bound() override;
    void sync() override;
    void unbind() override;

public:
    void attach_to_fbo(const GLenum target, const GLenum attachment) override;

};

}

#endif
