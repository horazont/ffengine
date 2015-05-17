/**********************************************************************
File name: common.hpp
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
#ifndef SCC_ENGINE_IO_COMMON_H
#define SCC_ENGINE_IO_COMMON_H

#include <chrono>
#include <memory>

namespace io {

class Mount;
typedef std::unique_ptr<Mount> MountPtr;

enum class MountPriority
{
    PracticallyInexistant = -3,
    Discriminated = -2,
    Fallback = -1,
    FileSystem = 0,
    Important = 1,
    Override = 2,
    Penetrant = 3
};

enum VFSStatMode
{
    VSM_UNUSED =        0x0000001,
    VSM_WRITABLE =      0x0000002,
    VSM_READABLE =      0x0000004,
    VSM_DIRECTORY =     0x0040000,
    VSM_REGULAR =       0x0100000
};

struct VFSStat
{
    Mount *mount;
    uint32_t mode;
    size_t size;
    std::chrono::system_clock::time_point mtime;
};

}

#endif
