#ifndef SCC_ENGINE_GL_SHADER_H
#define SCC_ENGINE_GL_SHADER_H

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/gl/object.hpp"
#include "engine/gl/ubo.hpp"

#include "engine/io/log.hpp"


namespace engine {

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


template <int N, typename ubo_t>
struct typecheck_helper
{
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

        typecheck_helper<N+1, ubo_t>::typecheck(block, member_idx, offset);
    }
};

template <typename ubo_t>
struct typecheck_helper<std::tuple_size<typename ubo_t::local_types>::value, ubo_t>
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

        if (total_members != ubo_t::storage_t::nelements) {
            throw std::runtime_error("inconsistent number of members (" +
                                     std::to_string(total_members) + " on gpu, " +
                                     std::to_string(ubo_t::storage_t::nelements) +
                                     " locally)");
        }

        typecheck_helper<0, ubo_t>::typecheck(block, 0, 0);
    }

public:
    bool attach(GLenum shader_type, const std::string &source);
    GLint attrib_location(const std::string &name) const;
    void bind_uniform_block(const std::string &name, const GLuint index);
    bool link();
    GLint uniform_location(const std::string &name) const;
    GLint uniform_block_location(const std::string &name) const;

public:
    inline const std::vector<ShaderVertexAttribute> &attributes() const
    {
        return m_attribs;
    }

public:
    template <typename ubo_t>
    inline void check_uniform_block(const std::string &block_name, const ubo_t &ubo)
    {
        auto iter = m_uniform_blocks.find(block_name);
        if (iter == m_uniform_blocks.end()) {
            throw std::invalid_argument("no such uniform block: "+block_name);
        }

        check_uniform_block_impl<ubo_t>(iter->second);
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
