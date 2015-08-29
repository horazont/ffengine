/**********************************************************************
File name: material.hpp
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
#ifndef SCC_ENGINE_GL_MATERIAL_H
#define SCC_ENGINE_GL_MATERIAL_H

#include <cassert>
#include <unordered_map>

#include "ffengine/gl/shader.hpp"
#include "ffengine/gl/texture.hpp"
#include "ffengine/gl/vbo.hpp"
#include "ffengine/gl/ibo.hpp"
#include "ffengine/gl/vao.hpp"


namespace engine {

class Material: public Resource
{
public:
    struct TextureAttachment
    {
        std::string name;
        GLint texture_unit;
        _GLObjectBase *texture_obj;
    };

public:
    /**
     * Create a new material with the given VBOFormat.
     *
     * @param format Format to use for the VBO of the material.
     */
    explicit Material(const VBOFormat &format);

    /**
     * Create a new material which uses the given VBO and IBO.
     *
     * The material will not take ownership neither of the VBO nor of the IBO
     * and will free neither.
     *
     * @param vbo The VBO to use
     * @param ibo The IBO to use
     */
    Material(VBO &vbo, IBO &ibo);

    Material(const Material &ref) = delete;
    Material(Material &&src) = delete;

    Material &operator=(const Material &ref) = delete;
    Material &operator=(Material &&src) = delete;

    ~Material() override;

private:
    const GLint m_max_texture_units;
    ShaderProgram m_shader;

    VBO *m_vbo;
    IBO *m_ibo;
    bool m_buffers_shared;
    ArrayDeclaration m_declaration;

    std::unordered_map<std::string, TextureAttachment> m_texture_bindings;
    std::vector<GLint> m_free_units;
    GLint m_base_free_unit;

    std::unique_ptr<VAO> m_vao;
protected:
    GLint get_next_texture_unit();

public:
    inline ShaderProgram &shader()
    {
        return m_shader;
    }

public:
    GLint attach_texture(const std::string &name, Texture2D *tex);

    inline const ArrayDeclaration &declaration() const
    {
        return m_declaration;
    }

    /**
     * Declare an attribute into the array declaration().
     *
     * This must not be called after link() has been called.
     *
     * @param name The name of the vertex attribute as used in the shader.
     * @param vbo_attr The index of the attribute inside the vbo().
     * @param normalized Whether the attribute shall be normalized (see the
     * corresponding argument to glVertexAttribPointer).
     */
    void declare_attribute(const std::string &name,
                           unsigned int vbo_attr,
                           bool normalized = false);

    void detach_texture(const std::string &name);

    /**
     * Link the shader and the array declaration together.
     *
     * The shader must not be linked before the call to this method. All vertex
     * attributes need to be declared beforehands using declare_attribute().
     *
     * If the array declaration misses attributes required by the shader,
     * std::runtime_error is thrown.
     *
     * Textures may be attached only after this method has been called as
     * attaching a texture requires a linked shader.
     *
     * @return true if all linking succeeded, false otherwise.
     *
     * @opengl
     */
    bool link();

    /**
     * Return a pointer to the VAO used by this material, if any.
     *
     * This is only available after link() was successful and it returns NULL
     * otherwise.
     */
    VAO *vao()
    {
        return m_vao.get();
    }

    /**
     * The VBO used by the material.
     */
    VBO &vbo()
    {
        return *m_vbo;
    }

    /**
     * The IBO used by the material.
     */
    IBO &ibo()
    {
        return *m_ibo;
    }

public:
    /**
     * Bind the shader, the VAO and all textures required by the material.
     *
     * @warning As this commands binds the VAO, great care needs to be taken.
     * Unless for rendering, you should **never** call this function and
     * instead use shader() or vbo() or ibo(), depending on your needs. Calling
     * sync() is also safe, as it only binds the vbo() and the ibo(), but not
     * the VAO.
     *
     * @opengl
     */
    void bind();

    /**
     * Synchronize the buffers to the remote.
     *
     * This is a no-op if the buffers are shared. It is the owners
     * responsibility to synchronize the buffers.
     */
    void sync();

public:
    static inline std::unique_ptr<Material> shared_with(Material &ref)
    {
        return std::make_unique<Material>(ref.vbo(), ref.ibo());
    }

};

}

#endif
