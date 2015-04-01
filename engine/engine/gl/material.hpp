#ifndef SCC_ENGINE_GL_MATERIAL_H
#define SCC_ENGINE_GL_MATERIAL_H

#include <cassert>
#include <unordered_map>

#include "engine/gl/shader.hpp"
#include "engine/gl/texture.hpp"


namespace engine {

class Material: public Resource
{
public:
    struct TextureAttachment
    {
        std::string name;
        GLint texture_unit;
        Texture *texture_obj;
    };

public:
    Material();

private:
    const GLint m_max_texture_units;
    ShaderProgram m_shader;
    std::unordered_map<std::string, TextureAttachment> m_texture_bindings;
    std::vector<GLint> m_free_units;
    GLint m_base_free_unit;

protected:
    GLint get_next_texture_unit();

public:
    inline ShaderProgram &shader()
    {
        return m_shader;
    }

public:
    GLint attach_texture(const std::string &name, Texture *tex);
    void detach_texture(const std::string &name);

};

}

#endif
