/**********************************************************************
File name: vbo.hpp
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
#ifndef SCC_ENGINE_GL_VBO_H
#define SCC_ENGINE_GL_VBO_H

#include <cstdint>

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <GL/glew.h>

#include "ffengine/gl/object.hpp"
#include "ffengine/gl/array.hpp"


namespace ffe {

class VBOAttribute
{
public:
    VBOAttribute(unsigned int length);

public:
    const unsigned int length;

};


typedef std::vector<VBOAttribute> VBOFormat;


class VBOFinalAttribute: public VBOAttribute
{
public:
    VBOFinalAttribute(
        const VBOAttribute &which,
        const unsigned int element_size,
        const unsigned int offset);

public:
    const unsigned int offset;
    const unsigned int size;

};


class VBO: public GLArray<float, GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, VBO>
{
public:
    VBO(const VBOFormat &format);
    virtual ~VBO();

private:
    std::vector<VBOFinalAttribute> m_attrs;

public:
    inline const std::vector<VBOFinalAttribute> &attrs() const
    {
        return m_attrs;
    }

    inline unsigned int vertex_size() const
    {
        return m_block_length * sizeof(element_t);
    }

};

typedef VBO::allocation_t VBOAllocation;

template <typename item_t, typename element_t = float>
class VBOSlice
{
public:
    VBOSlice(const VBOAllocation &base,
             const unsigned int nattr):
        m_base_ptr(base.get()),
        m_block_length(base.elements_per_block()),
        m_offset_in_block(static_cast<VBO*>(base.buffer())->attrs().at(nattr).offset / sizeof(element_t)),
        m_items(base.length())
    {

    }

private:
    element_t *const m_base_ptr;
    const unsigned int m_block_length;
    const unsigned int m_offset_in_block;
    const unsigned int m_items;

    inline item_t *get(const unsigned int n)
    {
        element_t *const start_element = m_base_ptr +
                m_block_length*n +
                m_offset_in_block;
        return reinterpret_cast<item_t*>(start_element);
    }

public:
    inline item_t &at(const unsigned int i)
    {
        if (i < 0 || i >= m_items)
        {
            throw std::out_of_range(std::string("index out of bounds: ")+std::to_string(i));
        }

        return (*this)[i];
    }

    inline item_t &operator[](const unsigned int i)
    {
        return *get(i);
    }

};

}


#endif
