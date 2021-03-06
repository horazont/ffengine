/**********************************************************************
File name: shader.hpp
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
#ifndef SCC_ENGINE_GL_SHADER_H
#define SCC_ENGINE_GL_SHADER_H

#include <sigc++/sigc++.h>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <QString>

#include <spp/spp.hpp>

#include "ffengine/gl/object.hpp"
#include "ffengine/gl/ubo.hpp"

#include "ffengine/io/log.hpp"


namespace ffe {

struct ShaderVertexAttribute
{
    ShaderVertexAttribute() = default;
    ShaderVertexAttribute(GLint loc, const std::string &name, GLenum type, GLint size);

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


template <int size_minus_N, typename ubo_t>
struct typecheck_helper
{
    static constexpr int N = std::tuple_size<typename ubo_t::local_types>::value - size_minus_N;

    static inline const ShaderUniformBlockMember &get_next_member(
            const ShaderUniformBlock &block,
            int &member_idx,
            int &offset)
    {
        const ShaderUniformBlockMember &result = block.members[member_idx];
        offset += 1;
        if (offset >= result.size) {
            member_idx += 1;
            offset = 0;
        }

        return result;
    }

    static inline void typecheck(const ShaderUniformBlock &block,
                                 int member_idx,
                                 int offset)
    {
        const int this_member = member_idx;
        const int this_offset = offset;

        const ShaderUniformBlockMember &member =
                get_next_member(block, member_idx, offset);

        typedef ubo_wrap_type<typename std::tuple_element<N, typename ubo_t::local_types>::type> wrap_helper;

        if (member.size != wrap_helper::nitems)
        {
            io::logging().get_logger("gl.shader").logf(
                        io::LOG_EXCEPTION,
                        "uniform typecheck: member %d:%d: OpenGL reports size "
                        "%d, UBO member reports %zu",
                        this_member,
                        this_offset,
                        member.size,
                        wrap_helper::nitems);
            throw std::runtime_error("inconsistent types at member "+std::to_string(this_member));
        }

        if (member.type != wrap_helper::gl_type)
        {
            io::logging().get_logger("gl.shader").logf(
                        io::LOG_EXCEPTION,
                        "uniform typecheck: member %d:%d: OpenGL reports type "
                        "0x%x, UBO member reports 0x%x",
                        this_member,
                        this_offset,
                        member.type,
                        wrap_helper::gl_type);
            throw std::runtime_error("inconsistent types at member "+std::to_string(this_member));
        }

        typecheck_helper<size_minus_N-1, ubo_t>::typecheck(block, member_idx, offset);
    }
};

template <typename ubo_t>
struct typecheck_helper<0, ubo_t>
{
    static inline void typecheck(const ShaderUniformBlock&,
                                 int,
                                 int)
    {

    }
};


class ShaderProgram: public GLObject<GL_CURRENT_PROGRAM>
{
public:
    ShaderProgram();

private:
    std::vector<ShaderVertexAttribute> m_attribs;
    std::unordered_map<std::string, ShaderVertexAttribute&> m_attrib_map;

    std::unordered_map<std::string, ShaderUniform> m_uniforms;
    std::unordered_map<std::string, ShaderUniformBlock> m_uniform_blocks;

protected:
    bool compile(GLint shader_object,
                 const char *source,
                 const GLint source_len,
                 const QString &filename);
    bool create_and_compile_and_attach(GLenum type,
                                       const char *source,
                                       const GLint source_len,
                                       const QString &filename);
    void delete_globject() override;
    void introspect();
    void introspect_vertex_attributes();
    void introspect_uniforms();

protected:
    template <typename ubo_t>
    inline void check_uniform_block_impl(ShaderUniformBlock &block)
    {
        std::size_t total_members = 0;
        for (auto &member: block.members) {
            total_members += member.size;
        }

        if (total_members != ubo_t::storage_t::nitems) {
            throw std::runtime_error("inconsistent number of members (" +
                                     std::to_string(total_members) + " on gpu, " +
                                     std::to_string(ubo_t::storage_t::nelements) +
                                     " locally)");
        }

        typecheck_helper<std::tuple_size<typename ubo_t::local_types>::value, ubo_t>::typecheck(block, 0, 0);
    }

public:
    bool attach(GLenum shader_type, const std::string &source);
    bool attach(const spp::Program &program,
                spp::EvaluationContext &context,
                GLenum shader_type = 0);
    bool attach_resource(GLenum shader_type, const QString &filename);
    GLint attrib_location(const std::string &name) const;

    /**
     * Bind a uniform block index to a declaration.
     *
     * The shader does not need to be bound for this operation and this
     * operation does not change GL_CURRENT_PROGRAM.
     *
     * @param name Name of the uniform block declaration in the shader source.
     * @param index Index of the Uniform Block Binding.
     */
    void bind_uniform_block(const std::string &name, const GLuint index);

    bool link();
    GLint uniform_location(const std::string &name) const;
    GLint uniform_block_location(const std::string &name) const;
    const ShaderUniform &uniform(const std::string &name) const;

public:
    inline const std::vector<ShaderVertexAttribute> &attributes() const
    {
        return m_attribs;
    }

public:
    template <typename ubo_t>
    inline void check_uniform_block(const std::string &block_name, const ubo_t &)
    {
        check_uniform_block<ubo_t>(block_name);
    }

    template <typename ubo_t>
    inline void check_uniform_block(const std::string &block_name)
    {
        auto iter = m_uniform_blocks.find(block_name);
        if (iter == m_uniform_blocks.end()) {
            throw std::invalid_argument("no such uniform block: "+block_name);
        }

        check_uniform_block_impl<ubo_t>(iter->second);
    }

public:
    void bind() override;
    void sync() override;
    void unbind() override;

};

}

#endif
