#include "ffengine/gl/debug.hpp"

namespace ffe {

io::LogLevel severity_to_level(const GLenum severity)
{
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH: return io::LOG_ERROR;
    case GL_DEBUG_SEVERITY_MEDIUM: return io::LOG_WARNING;
    case GL_DEBUG_SEVERITY_LOW: return io::LOG_INFO;
    case GL_DEBUG_SEVERITY_NOTIFICATION: return io::LOG_DEBUG;
    default: return io::LOG_EXCEPTION;
    }
}

const char *type_to_str(const GLenum type)
{
    switch (type) {
    case GL_DEBUG_TYPE_ERROR: return "error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "deprecated";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "undefined";
    case GL_DEBUG_TYPE_PORTABILITY: return "portability";
    case GL_DEBUG_TYPE_PERFORMANCE: return "performance";
    case GL_DEBUG_TYPE_MARKER: return "marker";
    case GL_DEBUG_TYPE_PUSH_GROUP: return "push";
    case GL_DEBUG_TYPE_POP_GROUP: return "pop";
    case GL_DEBUG_TYPE_OTHER: return "other";
    default: return "unknown";
    }
}

const char *source_to_str(const GLenum source)
{
    switch (source) {
    case GL_DEBUG_SOURCE_API: return "api";
    case GL_DEBUG_SOURCE_APPLICATION: return "application";
    case GL_DEBUG_SOURCE_OTHER: return "other";
    case GL_DEBUG_SOURCE_SHADER_COMPILER: return "shader";
    case GL_DEBUG_SOURCE_THIRD_PARTY: return "third-party";
    default: return "unknown";
    }
}

void debug_to_log(GLenum source, GLenum type, GLuint id, GLenum severity,
                  GLsizei length, const GLchar *message, void *userParam)
{
    io::Logger &logger = *static_cast<io::Logger*>(userParam);
    std::string tmp;
    const char *msg_null_terminated = message;
    if (length >= 0) {
        tmp = std::string(message, length);
        msg_null_terminated = tmp.c_str();
    }
    logger.logf(severity_to_level(severity),
                "[%s] %s: (%u) %s",
                type_to_str(type),
                source_to_str(source),
                id,
                msg_null_terminated);
}

void send_gl_debug_to_logger(io::Logger &logger)
{
    glDebugMessageCallback(&debug_to_log, &logger);
}

}
