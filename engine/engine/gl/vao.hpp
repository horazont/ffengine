#ifndef SCC_ENGINE_GL_VAO_H
#define SCC_ENGINE_GL_VAO_H

#include "engine/gl/object.hpp"
#include "engine/gl/vbo.hpp"
#include "engine/gl/ibo.hpp"
#include "engine/gl/shader.hpp"


namespace engine {

class VAO: public GLObject<GL_VERTEX_ARRAY_BINDING>
{
public:
    VAO();

private:
    IBO *m_ibo_hint;
    std::vector<VBO*> m_vbo_hints;

protected:
    void delete_globject() override;

public:
    void add_vbo_hint(VBO *vbo_hint);
    void set_ibo_hint(IBO *ibo_hint);

public:
    void bind() override;
    void bound() override;
    void sync() override;
    void unbind() override;

};

struct AttributeMapping
{
    VBO &vbo;
    unsigned int attr_index;
    bool normalized;
};

class ArrayDeclaration
{
public:
    ArrayDeclaration();

private:
    IBO *m_ibo;
    std::unordered_map<std::string, AttributeMapping> m_attribs;

public:
    void declare_attribute(const std::string &name,
                           VBO &vbo,
                           unsigned int vbo_attr,
                           bool normalized=false);

    inline IBO *ibo() const
    {
        return m_ibo;
    }

    std::unique_ptr<VAO> make_vao(const ShaderProgram &for_shader,
                                  bool add_vbo_hints=false);

    void set_ibo(IBO *ibo);

};

}

#endif
