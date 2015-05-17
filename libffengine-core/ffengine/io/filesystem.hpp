/**********************************************************************
File name: filesystem.hpp
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
#ifndef SCC_ENGINE_IO_FILESYSTEM_H
#define SCC_ENGINE_IO_FILESYSTEM_H

#include <map>
#include <list>
#include <vector>
#include <memory>

#include "ffengine/io/common.hpp"
#include "ffengine/io/stream.hpp"
#include "ffengine/io/filestream.hpp"

namespace io {

class FileSystem
{
public:
    FileSystem();
    ~FileSystem();

private:
    typedef std::pair<std::string, MountPtr> MountItem;
    typedef std::vector<MountItem> MountList;

private:
    std::map<MountPriority, MountList, std::greater<MountPriority>> _mounts;

protected:
    std::pair<MountPriority, MountList::iterator> find_mount(
        const Mount* mount);

    void iter_file_mounts(
        const std::string &path,
        const std::function<bool (Mount*, const std::string&)> &handler);
    void sort_mount_list(MountList &list);

public:
    void listdir(const std::string &path,
                 std::vector<std::string> &items);

    void mount(
        const std::string &mount_point,
        MountPtr &&mount,
        MountPriority priority);

    std::unique_ptr<Stream> open(
            const std::string &path,
            const OpenMode openmode,
            const WriteMode writemode = WriteMode::IGNORE
            );

    void stat(const std::string &path,
              VFSStat &stat);
};

}

#endif
