/**********************************************************************
File name: errors.hpp
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
