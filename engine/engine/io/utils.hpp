/**********************************************************************
File name: utils.hpp
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
#ifndef SCC_ENGINE_IO_UTILS_H
#define SCC_ENGINE_IO_UTILS_H

#include <string>

namespace io {

std::string absolutify(const std::string &path);
std::string basename(const std::string &path);
std::string dirname(const std::string &path);
std::string join(const std::initializer_list<std::string> &segments);
std::string normalize_vfs_path(const std::string &path);
std::pair<std::string, std::string> splitext(const std::string &path);
void validate_vfs_path(const std::string &path);

}

#endif
