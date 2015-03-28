#include "vbo.h"

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
