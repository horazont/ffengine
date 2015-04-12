#include "engine/gl/shader.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <unordered_set>

#include <QFile>

#include "engine/gl/util.hpp"

#include "engine/io/log.hpp"


namespace engine {

io::Logger &shader_logger = io::logging().get_logger("gl.shader");

ShaderVertexAttribute::ShaderVertexAttribute(
        GLint loc,
        const std::string &name,
        GLenum type,
        GLint size):
    loc(loc),
    name(name),
    type(type),
    size(size)
{

}


ShaderUniformBlockMember::ShaderUniformBlockMember(
        GLenum type,
        GLint size,
        GLsizei offset,
        bool row_major):
    type(type),
    size(size),
    offset(offset),
    row_major(row_major)
{

}


ShaderProgram::ShaderProgram():
    GLObject<GL_CURRENT_PROGRAM>()
{
    m_glid = glCreateProgram();
}

bool ShaderProgram::compile(GLint shader_object,
                            const char *source,
                            const GLint source_len,
                            const QString &filename)
{
    glShaderSource(shader_object, 1, &source, &source_len);
    raise_last_gl_error();
    glCompileShader(shader_object);
    raise_last_gl_error();

    GLint status = 0;
    glGetShaderiv(shader_object, GL_COMPILE_STATUS, &status);
    raise_last_gl_error();

    GLint len = 0;
    glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &len);
    raise_last_gl_error();
    if (len > 1) {
        const io::LogLevel level = (status != GL_TRUE ? io::LOG_ERROR : io::LOG_WARNING);
        if (status != GL_TRUE) {
            shader_logger.log(level) << m_glid << ": " << filename.toStdString()
                                     << ": shader failed to compile"
                                     << io::submit;
        } else {
            shader_logger.log(level) << m_glid << ": " << filename.toStdString()
                                     << ": shader compiled with warnings"
                                     << io::submit;
        }

        std::string log;
        log.resize(len);
        glGetShaderInfoLog(shader_object, log.size(), &len, &log.front());
        // FIXME: split the output into lines and submit each line individually
        shader_logger.log(level) << m_glid << ": " << filename.toStdString()
                                 << ": "
                                 << log
                                 << io::submit;
    } else {
        shader_logger.log(io::LOG_INFO) << m_glid << ": " << filename.toStdString()
                                        << ": shader compiled successfully"
                                        << io::submit;
    }

    return status == GL_TRUE;
}

bool ShaderProgram::create_and_compile_and_attach(
        GLenum type,
        const char *source,
        const GLint source_len,
        const QString &filename)
{
    const GLenum shader = glCreateShader(type);
    if (!compile(shader, source, source_len, filename))
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

void ShaderProgram::delete_globject()
{
    glDeleteProgram(m_glid);
}

void ShaderProgram::introspect()
{
    m_attribs.clear();
    m_uniforms.clear();
    m_uniform_blocks.clear();
    introspect_vertex_attributes();
    introspect_uniforms();
}

void ShaderProgram::introspect_vertex_attributes()
{
    GLint max_length = 0;
    glGetProgramiv(m_glid, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &max_length);
    GLint active_attrs = 0;
    glGetProgramiv(m_glid, GL_ACTIVE_ATTRIBUTES, &active_attrs);

    std::string buf(max_length, '\0');
    for (int i = 0; i < active_attrs; i++) {
        GLsizei actual_length = 0;
        GLint nelements = 0;
        GLenum type = 0;
        buf.resize(max_length);
        glGetActiveAttrib(m_glid, i, buf.size(),
                          &actual_length,
                          &nelements,
                          &type,
                          &buf.front());
        buf.resize(actual_length);

        GLint loc = glGetAttribLocation(m_glid, buf.data());
        assert(loc >= 0);

        if (buf.size() > 0) {
            m_attribs.emplace_back(loc, buf, type, nelements);
            ShaderVertexAttribute &attr = *(m_attribs.end() - 1);
            m_attrib_map.emplace(buf, attr);
        }
    }
}

void ShaderProgram::introspect_uniforms()
{
    static const std::unordered_set<GLenum> matrix_types({
                                                             GL_FLOAT_MAT2,
                                                             GL_FLOAT_MAT3,
                                                             GL_FLOAT_MAT4,
                                                             GL_FLOAT_MAT2x3,
                                                             GL_FLOAT_MAT2x4,
                                                             GL_FLOAT_MAT3x2,
                                                             GL_FLOAT_MAT3x4,
                                                             GL_FLOAT_MAT4x2,
                                                             GL_FLOAT_MAT4x3,
                                                             GL_DOUBLE_MAT2,
                                                             GL_DOUBLE_MAT3,
                                                             GL_DOUBLE_MAT4,
                                                             GL_DOUBLE_MAT2x3,
                                                             GL_DOUBLE_MAT2x4,
                                                             GL_DOUBLE_MAT3x2,
                                                             GL_DOUBLE_MAT3x4,
                                                             GL_DOUBLE_MAT4x2,
                                                             GL_DOUBLE_MAT4x3
                                                         });

    GLint active_uniforms = 0;
    glGetProgramiv(m_glid, GL_ACTIVE_UNIFORMS, &active_uniforms);

    GLint active_blocks = 0;
    glGetProgramiv(m_glid, GL_ACTIVE_UNIFORM_BLOCKS, &active_blocks);

    for (GLint i = 0; i < active_blocks; i++)
    {
        GLint name_length;
        GLuint uniform_count;
        glGetActiveUniformBlockiv(m_glid, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &name_length);
        glGetActiveUniformBlockiv(m_glid, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, (GLint*)&uniform_count);

        std::string name(name_length-1, ' ');
        glGetActiveUniformBlockName(m_glid, i, name.size()+1, nullptr, &name.front());

        ShaderUniformBlock &block = m_uniform_blocks.emplace(name, ShaderUniformBlock()).first->second;
        block.loc = i;
        block.name = name;

        std::basic_string<GLuint> indicies(uniform_count, 0);
        glGetActiveUniformBlockiv(m_glid, i, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, (GLint*)&indicies.front());

        std::basic_string<GLenum> types(uniform_count, 0);
        std::basic_string<GLint> sizes(uniform_count, 0);
        std::basic_string<GLint> offsets(uniform_count, 0);
        std::basic_string<GLint> matrix_strides(uniform_count, 0);
        std::basic_string<GLint> row_majors(uniform_count, 0);

        glGetActiveUniformsiv(m_glid, uniform_count, indicies.data(), GL_UNIFORM_TYPE, (GLint*)&types.front());
        glGetActiveUniformsiv(m_glid, uniform_count, indicies.data(), GL_UNIFORM_SIZE, &sizes.front());
        glGetActiveUniformsiv(m_glid, uniform_count, indicies.data(), GL_UNIFORM_OFFSET, &offsets.front());
        glGetActiveUniformsiv(m_glid, uniform_count, indicies.data(), GL_UNIFORM_MATRIX_STRIDE, &matrix_strides.front());
        glGetActiveUniformsiv(m_glid, uniform_count, indicies.data(), GL_UNIFORM_IS_ROW_MAJOR, &row_majors.front());

        for (GLuint j = 0; j < uniform_count; j++) {
            // FIXME: fix this check. strides count existing elements, so for
            // a mat4f we expect 16.

            GLint name_length = 0;
            glGetActiveUniformsiv(m_glid, 1, &j, GL_UNIFORM_NAME_LENGTH, &name_length);
            std::string name(name_length, ' ');
            glGetActiveUniformName(m_glid, j, name.size(), nullptr, &name.front());

            const bool is_matrix = matrix_types.find(types[j]) != matrix_types.end();

            shader_logger.logf(io::LOG_DEBUG,
                               "block %d uniform %d (%s): "
                               "type = 0x%x (is_matrix=%d), "
                               "size = %d, "
                               "offset = %d, "
                               "matrix_stride = %d",
                               i, j,
                               name.data(),
                               types[j],
                               (is_matrix ? 1 : 0),
                               sizes[j],
                               offsets[j],
                               matrix_strides[j]);

            /* if (matrix_strides[j] > 0) {
                throw std::runtime_error("unsupported matrix stride ("+
                                         std::to_string(matrix_strides[j])+
                                         ") for uniform "+
                                         name);
            } */

            if (row_majors[j] != GL_TRUE && is_matrix)
            {
                throw std::runtime_error("matrices in UBOs must be row major");
            }

            block.members.emplace_back(
                        types[j], sizes[j], offsets[j],
                        row_majors[j] == GL_TRUE);
        }

        std::sort(block.members.begin(),
                  block.members.end(),
                  [](const ShaderUniformBlockMember &m1,
                     const ShaderUniformBlockMember &m2)
                  {
                      return m1.offset < m2.offset;
                  });
    }

    for (GLuint i = 0; i < (GLuint)active_uniforms; i++)
    {
        GLint block_index;
        glGetActiveUniformsiv(m_glid, 1, &i, GL_UNIFORM_BLOCK_INDEX, &block_index);
        if (block_index > 0) {
            continue;
        }

        GLint type, size, name_length;
        glGetActiveUniformsiv(m_glid, 1, &i, GL_UNIFORM_TYPE, &type);
        glGetActiveUniformsiv(m_glid, 1, &i, GL_UNIFORM_SIZE, &size);
        glGetActiveUniformsiv(m_glid, 1, &i, GL_UNIFORM_NAME_LENGTH, &name_length);
        raise_last_gl_error();

        std::string name(name_length-1, ' ');
        glGetActiveUniformName(m_glid, i, name.size()+1, nullptr, &name.front());

        ShaderUniform &uniform = (m_uniforms[name] = ShaderUniform());
        uniform.loc = i;
        uniform.name = name;
        uniform.size = size;
        uniform.type = type;
    }
}

bool ShaderProgram::attach(GLenum shader_type, const std::string &source)
{
    return create_and_compile_and_attach(shader_type,
                                         source.c_str(),
                                         source.size()+1,
                                         "<memory>");
}

bool ShaderProgram::attach_resource(GLenum shader_type, const QString &filename)
{
    QFile source_file(filename);
    if (!source_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw std::runtime_error("failed to open shader resource: "+filename.toStdString());
    }
    QByteArray data = source_file.readAll();
    data.append("\0");
    shader_logger.logf(io::LOG_DEBUG, "compiling shader from resource %s: %s",
                       filename.toStdString().c_str(),
                       data.constData());
    return create_and_compile_and_attach(shader_type,
                                         data.constData(),
                                         data.size(),
                                         filename);
}

GLint ShaderProgram::attrib_location(const std::string &name) const
{
    auto iter = m_attrib_map.find(name);
    if (iter != m_attrib_map.end())
    {
        return iter->second.loc;
    }

    return -1;
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
    GLint status = 0;
    glGetProgramiv(m_glid, GL_LINK_STATUS, &status);

    GLint len = 0;
    glGetProgramiv(m_glid, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        io::LogLevel level = io::LOG_DEBUG;
        if (status == GL_TRUE) {
            level = io::LOG_WARNING;
            shader_logger.log(level) << m_glid << ": program linked with warnings"
                                     << io::submit;
        } else {
            level = io::LOG_ERROR;
            shader_logger.log(level) << m_glid << ": program failed to link"
                                     << io::submit;
        }

        std::string log;
        log.resize(len);
        glGetProgramInfoLog(m_glid, log.size(), &len, &log.front());
        shader_logger.log(level) << m_glid << ": \n" << log << io::submit;
    }

    if (status != GL_TRUE) {
        return false;
    }

    introspect();

    return true;
}

const ShaderUniform &ShaderProgram::uniform(const std::string &name) const
{
    auto iter = m_uniforms.find(name);
    if (iter != m_uniforms.end()) {
        return iter->second;
    }

    throw std::runtime_error("no such uniform: "+name);
}

GLint ShaderProgram::uniform_location(const std::string &name) const
{
    auto iter = m_uniforms.find(name);
    if (iter != m_uniforms.end())
    {
        return iter->second.loc;
    }

    shader_logger.logf(io::LOG_DEBUG, "inactive uniform requested: %s",
                       name.data());

    return -1;
}

GLint ShaderProgram::uniform_block_location(const std::string &name) const
{
    auto iter = m_uniform_blocks.find(name);
    if (iter != m_uniform_blocks.end()) {
        return iter->second.loc;
    }

    return -1;
}

void ShaderProgram::bind()
{
    glUseProgram(m_glid);
}

void ShaderProgram::sync()
{

}

void ShaderProgram::unbind()
{
    glUseProgram(0);
}

}
