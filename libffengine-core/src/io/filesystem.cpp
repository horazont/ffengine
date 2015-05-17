/**********************************************************************
File name: filesystem.cpp
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
#include "ffengine/io/filesystem.hpp"

#include <algorithm>
#include <iostream>

#include "ffengine/io/errors.hpp"
#include "ffengine/io/mount.hpp"
#include "ffengine/io/utils.hpp"

namespace io {

/* io::FileSystem */

FileSystem::FileSystem():
    _mounts()
{

}

FileSystem::~FileSystem()
{

}

void FileSystem::iter_file_mounts(
    const std::string &path,
    const std::function<bool (Mount*, const std::string&)> &handler)
{
    for (auto &prio_list: _mounts)
    {
        MountList &list = prio_list.second;
        for (auto &path_mount: list)
        {
            std::string &mountpoint = path_mount.first;
            if (path.compare(0, mountpoint.size(), mountpoint) != 0) {
                continue;
            }
            bool finish = handler(
                path_mount.second.get(),
                path.substr(mountpoint.size()+1));
            if (finish) {
                return;
            }
        }
    }
}

void FileSystem::sort_mount_list(FileSystem::MountList &list)
{
    std::sort(list.begin(), list.end(),
        [](const MountItem &a, const MountItem &b){
            return a.first.size() < b.first.size();
        });
}

void FileSystem::listdir(
    const std::string &path, std::vector<std::string> &items)
{
    bool found = false;
    bool had_permission_denied = false;

    iter_file_mounts(
        path,
        [&found, &items, &had_permission_denied](
            Mount *mount,
            const std::string &local_path) mutable -> bool
        {
            try {
                mount->listdir(local_path, items);
            } catch (const VFSPermissionDeniedError &err) {
                had_permission_denied = true;
                return false;
            } catch (const VFSIOError &err) {
                return false;
            }
            found = true;
            return true;
        });

    if (!found) {
        if (had_permission_denied) {
            throw VFSPermissionDeniedError(path);
        } else {
            throw VFSFileNotFoundError(path);
        }
    }
}

void FileSystem::mount(
        const std::string &mount_point,
        MountPtr &&mount,
        MountPriority priority)
{
    std::string path = normalize_vfs_path(mount_point);
    validate_vfs_path(path);

    _mounts[priority].push_back(std::make_pair(path, std::move(mount)));
    sort_mount_list(_mounts[priority]);
}

std::unique_ptr<Stream> FileSystem::open(
        const std::string &path,
        const OpenMode openmode,
        const WriteMode writemode)
{
    bool had_permission_denied = false;
    std::unique_ptr<Stream> result;

    iter_file_mounts(
        path,
        [&result, &had_permission_denied, openmode, writemode](
            Mount *mount,
            const std::string &local_path) mutable -> bool
        {
            try {
                result = mount->open(local_path, openmode, writemode);
            } catch (const VFSPermissionDeniedError &err) {
                had_permission_denied = true;
            } catch (const VFSIOError &err) {

            }
            if (result) {
                return true;
            }
            return false;
        });

    if (!result) {
        if (had_permission_denied) {
            throw VFSPermissionDeniedError(path);
        } else {
            throw VFSFileNotFoundError(path);
        }
    }
    return result;
}

void FileSystem::stat(const std::string &path, VFSStat &stat)
{
    bool had_permission_denied = false;
    bool success = false;

    iter_file_mounts(
        path,
        [&stat, &success, &had_permission_denied](
            Mount *mount,
            const std::string &local_path) mutable -> bool
        {
            try {
                mount->stat(local_path, stat);
                success = true;
                return true;
            } catch (const VFSPermissionDeniedError &err) {
                had_permission_denied = true;
                return false;
            } catch (const VFSIOError &err) {
                return false;
            }
            return false;
        });

    if (!success) {
        if (had_permission_denied) {
            throw VFSPermissionDeniedError(path);
        } else {
            throw VFSFileNotFoundError(path);
        }
    }
}

}
