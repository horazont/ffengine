#ifndef SCC_ENGINE_GL_SHADER_H
#define SCC_ENGINE_GL_SHADER_H

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "object.h"
#include "ubo.h"


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


template <int N, typename ubo_t, typename... element_ts>
struct typecheck_helper;

template <int N, typename ubo_t, typename element_t, typename... element_ts>
struct typecheck_helper<N, ubo_t, element_t, element_ts...>
{
    static inline ShaderUniformBlockMember &get_next_member(
            ShaderUniformBlock &block,
            int &member_idx,
            int &offset)
    {
        ShaderUniformBlockMember &result = block.members[member_idx];
        offset += 1;
        if (offset >= result.size) {
            member_idx += 1;
            offset = 0;
        }

        return result;
    }

    static void typecheck(ShaderUniformBlock &block,
                          int member_idx,
                          int offset,
                          const ubo_t &ubo)
    {
        typedef ubo_wrap_type<element_t> wrap_helper;

        ShaderUniformBlockMember &member = get_next_member(block,
                                                           member_idx,
                                                           offset);
        if (member.type != wrap_helper::gl_type) {
            throw std::runtime_error("inconsistent types at member "+std::to_string(member_idx));
        }

        typecheck_helper<N+1, ubo_t, element_ts...>::typecheck(block, member_idx, offset, ubo);
    }

};

template <int N, typename ubo_t>
struct typecheck_helper<N, ubo_t>
{
    static void typecheck(ShaderUniformBlock &, int, int, const ubo_t&)
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
    template <typename... element_ts>
    inline void check_uniform_block(ShaderUniformBlock &block, const UBO<element_ts...> &ubo)
    {
        typedef UBO<element_ts...> ubo_t;
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

        typecheck_helper<0, ubo_t, element_ts...>::typecheck(block, 0, 0, ubo);
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

        check_uniform_block(iter->second, ubo);
    }

public:
    void bind() override;
    void unbind() override;
};

}

#endif
