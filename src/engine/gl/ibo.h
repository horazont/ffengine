#ifndef SCC_ENGINE_GL_IBO_H
#define SCC_ENGINE_GL_IBO_H

#include "array.h"

namespace engine {

class IBO: public GLArray<uint32_t,
                          GL_ELEMENT_ARRAY_BUFFER,
                          GL_ELEMENT_ARRAY_BUFFER_BINDING>
{
public:
    IBO();

};

typedef IBO::allocation_t IBOAllocation;

}

#endif
