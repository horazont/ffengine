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
    static const unsigned int MAX_TEXTURES = 8;

public:
    Material();

private:
    ShaderProgram m_shader;
    std::array<Texture2D*, MAX_TEXTURES> m_texture_units;

public:
    inline ShaderProgram &shader()
    {
        return m_shader;
    }

    inline Texture2D *&texture_unit(unsigned int nunit)
    {
        assert(nunit < MAX_TEXTURES);
        return m_texture_units[nunit];
    }

};

}

#endif
