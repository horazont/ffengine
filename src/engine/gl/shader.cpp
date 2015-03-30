#include "shader.h"

#include <iostream>

#include "util.h"


namespace engine {

ShaderProgram::ShaderProgram():
    GLObject<GL_CURRENT_PROGRAM>()
{
    m_glid = glCreateProgram();
}

void ShaderProgram::delete_globject()
{
    glDeleteProgram(m_glid);
}

bool ShaderProgram::attach(GLenum shader_type, const std::string &source)
{
    const GLenum shader = glCreateShader(shader_type);
    const char *source_ptr = source.data();
    const GLint source_size = source.size();
    glShaderSource(shader, 1, &source_ptr, &source_size);
    raise_last_gl_error();
    glCompileShader(shader);
    raise_last_gl_error();

    GLint len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    raise_last_gl_error();
    if (len > 1) {
        std::string log;
        log.resize(len);
        glGetShaderInfoLog(shader, log.size(), &len, &log.front());
        std::cout << log << std::endl;
    }

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    raise_last_gl_error();
    if (status != GL_TRUE)
    {
        glDeleteShader(shader);
        return false;
    }

    glAttachShader(m_glid, shader);
    raise_last_gl_error();
    glDeleteShader(shader);
    raise_last_gl_error();

    return true;
}

GLint ShaderProgram::attrib_location(const std::string &name) const
{
    {
        auto iter = m_attrib_locs.find(name);
        if (iter != m_attrib_locs.end())
        {
            return iter->second;
        }
    }

    GLint loc = glGetAttribLocation(m_glid, name.data());
    if (loc < 0) {
        return loc;
    }

    m_attrib_locs.emplace(name, loc);
    return loc;
}

void ShaderProgram::bind_uniform_block(const std::string &name,
                                       const GLuint index)
{
    const GLint loc = uniform_block_location(name);
    if (loc < 0) {
        throw std::runtime_error("no such uniform block: " + name);
    }

    glUniformBlockBinding(m_glid, index, loc);
}

bool ShaderProgram::link()
{
    glLinkProgram(m_glid);
    GLint len = 0;
    glGetProgramiv(m_glid, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        std::string log;
        log.resize(len);
        glGetProgramInfoLog(m_glid, log.size(), &len, &log.front());
        std::cout << log << std::endl;
    }

    GLint status = 0;
    glGetProgramiv(m_glid, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        return false;
    }

    return true;
}

GLint ShaderProgram::uniform_location(const std::string &name) const
{
    {
        auto iter = m_uniform_locs.find(name);
        if (iter != m_uniform_locs.end())
        {
            return iter->second;
        }
    }

    GLint loc = glGetUniformLocation(m_glid, name.data());
    if (loc < 0) {
        return loc;
    }

    m_uniform_locs.emplace(name, loc);
    return loc;
}

GLint ShaderProgram::uniform_block_location(const std::string &name) const
{
    {
        auto iter = m_uniform_block_locs.find(name);
        if (iter != m_uniform_block_locs.end()) {
            return iter->second;
        }
    }

    GLint loc = glGetUniformBlockIndex(m_glid, name.data());
    if (loc < 0) {
        return loc;
    }

    m_uniform_block_locs.emplace(name, loc);
    return loc;
}

void ShaderProgram::bind()
{
    glUseProgram(m_glid);
}

void ShaderProgram::unbind()
{
    glUseProgram(0);
}

}
