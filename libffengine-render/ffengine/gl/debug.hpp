#ifndef SCC_GL_DEBUG_H
#define SCC_GL_DEBUG_H

#include <GL/glew.h>

#include "ffengine/io/log.hpp"


namespace ffe {

void send_gl_debug_to_logger(io::Logger &logger);

}


#endif
