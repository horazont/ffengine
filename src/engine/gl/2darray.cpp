#include "2darray.h"

GL2DArray::GL2DArray(const GLenum internal_format,
                     const GLsizei width,
                     const GLsizei height):
    m_internal_format(internal_format),
    m_width(width),
    m_height(height)
{

}
