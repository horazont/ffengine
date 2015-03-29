#ifndef SCC_ENGINE_GL_SHADER_H
#define SCC_ENGINE_GL_SHADER_H

#include "object.h"

#include <string>
#include <unordered_map>
#include <vector>


class ShaderProgram: public GLObject<GL_CURRENT_PROGRAM>
{
public:
    ShaderProgram();

private:
    std::vector<GLuint> m_pending_shaders;
    mutable std::unordered_map<std::string, GLint> m_attrib_locs;
    mutable std::unordered_map<std::string, GLint> m_uniform_locs;
    mutable std::unordered_map<std::string, GLint> m_uniform_block_locs;

protected:
    void delete_globject() override;

public:
    bool attach(GLenum shader_type, const std::string &source);
    GLint attrib_location(const std::string &name) const;
    void bind_uniform_block(const std::string &name, const GLuint index);
    bool link();
    GLint uniform_location(const std::string &name) const;
    GLint uniform_block_location(const std::string &name) const;

    void bind() override;
    void unbind() override;
};

#endif
