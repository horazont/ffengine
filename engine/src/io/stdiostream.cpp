#include "engine/io/stdiostream.hpp"

#include <unistd.h>

namespace io {

/* io::StdIOStream */

StdIOStream::StdIOStream(const int origFD):
    FDStream::FDStream(check_fd(dup(origFD)), true)
{
    
}

bool StdIOStream::is_seekable() const {
    return false;
}

/* io::StdInStream */

StdInStream::StdInStream():
    StdIOStream::StdIOStream(STDIN_FILENO)
{
    
}

bool StdInStream::is_readable() const {
    return true;
}

bool StdInStream::is_writable() const {
    return false;
}

/* io::StdOutStream */

StdOutStream::StdOutStream():
    StdIOStream::StdIOStream(STDOUT_FILENO)
{
    
}

bool StdOutStream::is_readable() const {
    return false;
}

bool StdOutStream::is_writable() const {
    return true;
}

/* io::StdErrStream */

StdErrStream::StdErrStream():
    StdIOStream::StdIOStream(STDERR_FILENO)
{
    
}

bool StdErrStream::is_readable() const {
    return false;
}

bool StdErrStream::is_writable() const {
    return true;
}

std::unique_ptr<Stream> stdin(new StdInStream());
std::unique_ptr<Stream> stdout(new StdOutStream());
std::unique_ptr<Stream> stderr(new StdErrStream());

}
