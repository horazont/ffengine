#ifndef SCC_ENGINE_GL_SHADER_H
#define SCC_ENGINE_GL_SHADER_H

#include "object.h"

#include <string>
#include <unordered_map>
#include <vector>


namespace engine {

struct ShaderVertexAttribute
{
    GLint loc;
    std::string name;
    GLenum type;
    GLint size;
};

struct ShaderUniform
{
    GLint loc;
    std::string name;
    GLenum type;
    GLint size;
};

struct ShaderUniformBlockMember
{
    ShaderUniformBlockMember() = default;
    ShaderUniformBlockMember(GLenum type, GLint size, GLsizei offset, bool row_major);

    GLenum type;
    GLint size;
    GLsizei offset;
    bool row_major;
};

struct ShaderUniformBlock
{
    GLint loc;
    std::string name;
    std::vector<ShaderUniformBlockMember> members;
};

class ShaderProgram: public GLObject<GL_CURRENT_PROGRAM>
{
public:
    ShaderProgram();

private:
    mutable std::unordered_map<std::string, ShaderVertexAttribute> m_attribs;
    mutable std::unordered_map<std::string, ShaderUniform> m_uniforms;
    mutable std::unordered_map<std::string, ShaderUniformBlock> m_uniform_blocks;

protected:
    void delete_globject() override;
    void introspect();
    void introspect_vertex_attributes();
    void introspect_uniforms();

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

}

#endif
