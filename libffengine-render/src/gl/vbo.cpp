/**********************************************************************
File name: vbo.cpp
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
#include "ffengine/gl/vbo.hpp"


namespace engine {

VBOAttribute::VBOAttribute(unsigned int length):
    length(length)
{

}


VBOFinalAttribute::VBOFinalAttribute(const VBOAttribute &which,
                                     const unsigned int element_size,
                                     unsigned int offset):
    VBOAttribute(which),
    offset(offset),
    size(element_size*which.length)
{

}


VBO::VBO(const VBOFormat &format):
    GLArray(),
    m_attrs()
{
    unsigned int offset = 0;
    for (auto &attr_decl: format)
    {
        m_attrs.emplace_back(
             attr_decl,
             sizeof(element_t),
             offset
        );
        offset += m_attrs[m_attrs.size()-1].size;
    }

    m_block_length = offset / sizeof(element_t);
}

VBO::~VBO()
{

}

}
