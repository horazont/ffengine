/**********************************************************************
File name: mount.hpp
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
#ifndef SCC_ENGINE_IO_MOUNT_H
#define SCC_ENGINE_IO_MOUNT_H

#include <memory>
#include <vector>

#include "ffengine/io/common.hpp"
#include "ffengine/io/stream.hpp"
#include "ffengine/io/filestream.hpp"

namespace io {

class Mount
{
public:
    Mount();
    virtual ~Mount();

public:
    virtual std::string get_real_path(const std::string &local_path);

    virtual void listdir(
        const std::string &local_path,
        std::vector<std::string> &items) = 0;

    virtual std::unique_ptr<Stream> open(
            const std::string &local_path,
            const OpenMode openmode,
            const WriteMode writemode) = 0;

    virtual void stat(const std::string &local_path, VFSStat &stat) = 0;
};

class MountDirectory: public Mount
{
public:
    MountDirectory(
        const std::string &fs_path,
        bool read_only = true);

private:
    const std::string _root;
    bool _read_only;

private:
    static std::string canonicalize_file_name(const char *name);
    static void handle_failure(const int err, const std::string &path);

public:
    std::string get_real_path(const std::string &local_path) override;
    void listdir(
        const std::string &local_path,
        std::vector<std::string> &items) override;
    std::unique_ptr<Stream> open(
            const std::string &local_path,
            const OpenMode openmode,
            const WriteMode writemode) override;
    void stat(const std::string &local_path, VFSStat &stat) override;
};

typedef std::unique_ptr<Mount> MountPtr;

}

#endif
