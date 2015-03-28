#ifndef SCC_ENGINE_GL_VAO_H
#define SCC_ENGINE_GL_VAO_H

#include "object.h"
#include "vbo.h"
#include "ibo.h"
#include "shader.h"

class VAO: public GLObject<GL_VERTEX_ARRAY_BINDING>
{
public:
    VAO(IBO *ibo_hint = nullptr);

private:
    IBO *m_ibo_hint;

protected:
    void delete_globject() override;

public:
    void set_ibo_hint(IBO *ibo_hint);

public:
    void bind() override;
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

    std::unique_ptr<VAO> make_vao(const ShaderProgram &for_shader);

    void set_ibo(IBO *ibo);

};

#endif
