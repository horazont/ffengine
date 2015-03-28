#ifndef SCC_ENGINE_GL_VBO_H
#define SCC_ENGINE_GL_VBO_H

#include <cstdint>

#include <vector>
#include <memory>
#include <unordered_map>

#include <GL/glew.h>

#include "object.h"
#include "array.h"


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


class VBO: public GLArray<float, GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING>
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

#endif
