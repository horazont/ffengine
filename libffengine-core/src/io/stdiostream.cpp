/**********************************************************************
File name: stdiostream.cpp
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
#include "ffengine/io/stdiostream.hpp"

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
