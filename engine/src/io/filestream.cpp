#include "engine/io/filestream.hpp"

#include <cerrno>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace io {

/* io::FDStream */

FDStream::FDStream(int fd, bool owns_fd):
    _fd(fd),
    _owns_fd(owns_fd)
{

}

FDStream::~FDStream() throw() {
    close();
}

void FDStream::flush() {
    fsync(_fd);
}

std::size_t FDStream::read(void *data, const std::size_t length) {
    std::ptrdiff_t readBytes = ::read(_fd, data, length);
    if (readBytes == -1) {
        engine::raise_last_os_error();
    }
    return readBytes;
}

std::size_t FDStream::seek(const int whence, const std::ptrdiff_t offset) {
    std::ptrdiff_t soughtOffset = lseek(_fd, offset, whence);
    if (offset == -1) {
        engine::raise_last_os_error();
    }
    return soughtOffset;
}

std::size_t FDStream::size() const {
    const std::ptrdiff_t pos = tell();
    const std::ptrdiff_t fsize = lseek(_fd, 0, SEEK_END);
    lseek(_fd, pos, SEEK_SET);
    return fsize;
}

std::size_t FDStream::tell() const {
    return lseek(_fd, 0, SEEK_CUR);
}

std::size_t FDStream::write(const void *data, const std::size_t length) {
    std::ptrdiff_t writtenBytes = ::write(_fd, data, length);
    if (writtenBytes == -1) {
        engine::raise_last_os_error();
    }
    return writtenBytes;
}

void FDStream::close() {
    if (_owns_fd) {
        ::close(_fd);
    }
    _fd = 0;
}

/* io::FileStream */

// note that throwing the exception on a failed open is done in checkFD
FileStream::FileStream(const std::string &filename,
                       const OpenMode openmode,
                       const WriteMode writemode,
                       const ShareMode sharemode):
    FDStream::FDStream(
        check_fd(open_file_with_modes(
                     filename,
                     openmode,
                     writemode,
                     sharemode)),
        true),
    _openmode(openmode)
{
    if (sharemode != ShareMode::DONT_CARE) {
        // log << ML_WARNING << "File stream for `" << fileName << "' opened with a ShareMode nonequal to SM_DONT_CARE. This is ignored." << submit;
    }
    struct stat fileStat;
    fileStat.st_mode = 0;
    fstat(_fd, &fileStat);
    _seekable = !(S_ISFIFO(fileStat.st_mode)
                  || S_ISSOCK(fileStat.st_mode)
                  || S_ISCHR(fileStat.st_mode));
}

bool FileStream::is_readable() const {
    return (_openmode != OpenMode::WRITE);
}

bool FileStream::is_seekable() const {
    return _seekable;
}

bool FileStream::is_writable() const {
    return (_openmode != OpenMode::READ);
}

/* free functions */


int open_file_with_modes(const std::string &filename,
                         const OpenMode openmode,
                         const WriteMode writemode,
                         const ShareMode sharemode)
{
    int flags = 0;

    switch (openmode) {
    case OpenMode::READ:
    {
        flags = O_RDONLY;
        break;
    }
    case OpenMode::WRITE:
    case OpenMode::BOTH:
    {
        flags = O_CREAT;
        switch (writemode) {
        case WriteMode::IGNORE:
        case WriteMode::OVERWRITE:
        {
            flags |= O_TRUNC;
        }
        case WriteMode::APPEND:
        {
            flags |= O_APPEND;
        }
        }

        if (openmode == OpenMode::BOTH) {
            flags |= O_RDWR;
        } else {
            flags |= O_WRONLY;
        }

        break;
    }
    }

    return open(
        filename.c_str(),
        flags,
        (S_IRWXU | S_IRWXG | S_IRWXO) & (~(S_IXUSR | S_IXGRP | S_IXOTH)));
}

}
