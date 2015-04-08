#ifndef SCC_ENGINE_GL_TEXTURE_H
#define SCC_ENGINE_GL_TEXTURE_H

#include <GL/glew.h>

#include "engine/gl/object.hpp"
#include "engine/gl/2darray.hpp"


namespace engine {

class Texture
{
public:
    virtual ~Texture();

public:
    virtual GLenum shader_uniform_type() = 0;
    virtual GLenum target() = 0;

};

class Texture2D: public GLObject<GL_TEXTURE_BINDING_2D>,
                 public GL2DArray,
                 public Texture
{
public:
    Texture2D(const GLenum internal_format,
              const GLsizei width,
              const GLsizei height,
              const GLenum init_format = 0,
              const GLenum init_type = GL_UNSIGNED_BYTE);

public:
    void bind() override;
    void bound() override;
    void sync() override;
    void unbind() override;

public:
    GLenum shader_uniform_type() override;
    GLenum target() override;

public:
    void attach_to_fbo(const GLenum target, const GLenum attachment) override;

};

}

#endif
