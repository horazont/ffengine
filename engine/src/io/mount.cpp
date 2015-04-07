#include "engine/io/mount.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#include <cerrno>
#include <cstring>
#include <fstream>

#include "engine/io/errors.hpp"
#include "engine/io/utils.hpp"

namespace io {

/* io::Mount */

Mount::Mount()
{

}

Mount::~Mount()
{

}

std::string Mount::get_real_path(const std::string &)
{
    return "";
}

/* io::MountDirectory */

MountDirectory::MountDirectory(
        const std::string &fs_path,
        bool read_only):
    _root(canonicalize_file_name(fs_path.c_str())),
    _read_only(read_only)
{

}

std::string MountDirectory::canonicalize_file_name(
    const char *name)
{
    // FIXME: portability
    char *canonical = realpath(name, nullptr);
    if (!canonical) {
        throw std::system_error(errno, std::system_category());
    }
    std::string result;
    try {
        result = canonical;
    } catch (...) {
        free(canonical);
        throw;
    }
    free(canonical);

    return result;
}

void MountDirectory::handle_failure(const int err, const std::string &path)
{
    switch (err) {
    case EACCES:
    {
        throw VFSPermissionDeniedError(path);
    }
    case ENOENT:
    {
        throw VFSFileNotFoundError(path);
    }
    default:
    {
        throw VFSIOError(std::strerror(err));
    }
    }
}

std::string MountDirectory::get_real_path(const std::string &local_path)
{
    return join({_root, local_path});
}

void MountDirectory::listdir(
    const std::string &local_path,
    std::vector<std::string> &items)
{
    const std::string full_dir_path = join({_root, local_path});
    // FIXME: portability
    DIR *dir = opendir(full_dir_path.c_str());
    if (dir == nullptr) {
        switch (errno) {
        case EACCES:
            throw VFSPermissionDeniedError(local_path);
        case ENOENT:
            throw VFSFileNotFoundError(local_path);
        default:
            throw VFSIOError(std::string("IO error: ") + strerror(errno));
        }
    }

    dirent *ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        items.push_back(std::string(ent->d_name));
    }
    closedir(dir);
}

std::unique_ptr<Stream> MountDirectory::open(
        const std::string &local_path,
        const OpenMode openmode,
        const WriteMode writemode)
{
    if (openmode != OpenMode::READ && _read_only) {
        throw VFSPermissionDeniedError(local_path);
    }

    const std::string full_dir_path = join({_root, local_path});

    try {
        return std::unique_ptr<Stream>(new FileStream(
                                           full_dir_path,
                                           openmode,
                                           writemode));
    } catch (const std::system_error &err) {
        switch (err.code().value())
        {
        case ENOENT:
        {
            throw VFSFileNotFoundError(local_path);
        }
        case EACCES:
        {
            throw VFSPermissionDeniedError(local_path);
        }
        default:
            throw VFSIOError(err.what());
        }
    }
}

void MountDirectory::stat(const std::string &local_path, VFSStat &stat)
{
    const std::string full_dir_path = join({_root, local_path});
    // FIXME: portability

    struct stat os_buf;
    int result = ::stat(full_dir_path.c_str(), &os_buf);
    if (result != 0) {
        switch (errno) {
        case EACCES:
            throw VFSPermissionDeniedError(local_path);
        case ENOENT:
            throw VFSFileNotFoundError(local_path);
        default:
            throw VFSIOError(std::string("IO error: ") + strerror(errno));
        }
    }

    stat.mount = this;
    stat.mode = 0;

    if (S_ISREG(os_buf.st_mode)) {
        stat.mode |= VSM_REGULAR;
    }
    if (S_ISDIR(os_buf.st_mode)) {
        stat.mode |= VSM_DIRECTORY;
    }

    stat.mtime = std::chrono::system_clock::from_time_t(
        os_buf.st_mtime);
    stat.size = os_buf.st_size;
}

}
