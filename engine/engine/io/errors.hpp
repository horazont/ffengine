#ifndef SCC_ENGINE_IO_ERRORS_H
#define SCC_ENGINE_IO_ERRORS_H

#include <stdexcept>

namespace io {

class VFSIOError: public std::runtime_error
{
public:
    VFSIOError(const std::string &what_arg);
    VFSIOError(const char *what_arg);

};

class VFSPermissionDeniedError: public VFSIOError
{
public:
    VFSPermissionDeniedError(const std::string &path);

};

class VFSFileNotFoundError: public VFSIOError
{
public:
    VFSFileNotFoundError(const std::string &path);

};

}

#endif
