#include "engine/io/errors.hpp"

namespace io {

/* io::VFSIOError */

VFSIOError::VFSIOError(const std::string &what_arg):
    std::runtime_error(what_arg)
{

}

VFSIOError::VFSIOError(const char *what_arg):
    std::runtime_error(what_arg)
{

}

/* io::VFSPermissionDeniedError */

VFSPermissionDeniedError::VFSPermissionDeniedError(
        const std::string &what_arg):
    VFSIOError("Permission denied: "+what_arg)
{

}

/* io::VFSFileNotFoundError */

VFSFileNotFoundError::VFSFileNotFoundError(
        const std::string &what_arg):
    VFSIOError("File not found: "+what_arg)
{

}

}
