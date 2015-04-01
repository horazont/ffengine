/**********************************************************************
File name: FileStream.hpp
This file is part of: Pythonic Engine

LICENSE

The contents of this file are subject to the Mozilla Public License
Version 1.1 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License at
http://www.mozilla.org/MPL/

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations under
the License.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public license (the  "GPL License"), in which case  the
provisions of GPL License are applicable instead of those above.

FEEDBACK & QUESTIONS

For feedback and questions about pyengine please e-mail one of the
authors named in the AUTHORS file.
**********************************************************************/
#ifndef SCC_ENGINE_IO_FILESTREAM_H
#define SCC_ENGINE_IO_FILESTREAM_H

#include "engine/common/utils.hpp"
#include "engine/io/stream.hpp"

namespace io {

static inline int check_fd(int fd) {
    if (fd == -1) {
        engine::raise_last_os_error();
    }
    return fd;
}


enum class WriteMode {
    IGNORE = 0,
    OVERWRITE = 1,
    APPEND = 2
};

enum class OpenMode {
    READ = 0,
    WRITE = 1,
    BOTH = 2
};

enum class ShareMode {
    EXCLUSIVE = 0,
    ALLOW_READ = 1,
    ALLOW_WRITE = 2,
    ALLOW_BOTH = 3,
    DONT_CARE = 4
};

class FileError: public StreamError {
public:
    FileError(const std::string message):
        StreamError(message) {};
    FileError(const char *message):
        StreamError(message) {};
    virtual ~FileError() throw() {};
};

class FDStream: public Stream {
public:
    FDStream(int fd, bool owns_fd = true);
    virtual ~FDStream() throw();

protected:
    int _fd;
    bool _owns_fd;

public:
    virtual void flush();
    inline int fileno() const { return _fd; };
    virtual std::size_t read(void *data, const std::size_t length) override;
    virtual std::size_t seek(const int whence, const std::ptrdiff_t offset) override;
    virtual std::size_t size() const override;
    virtual std::size_t tell() const override;
    virtual std::size_t write(const void *data, const std::size_t length) override;
    virtual void close() override;
};

/**
 * Opens a stream to access a file. This is how the OpenMode:s and
 * WriteMode:s map to open(2) modes.
 *
 * OpenMode     WriteMode       open flags
 * OM_READ      any             O_RDONLY
 * OM_WRITE     WM_IGNORE       O_WRONLY|O_TRUNC|O_CREAT
 * OM_WRITE     WM_OVERWRITE    O_WRONLY|O_TRUNC|O_CREAT
 * OM_WRITE     WM_APPEND       O_WRONLY|O_APPEND|O_CREAT
 * OM_BOTH      WM_IGNORE       O_RDWR|O_TRUNC|O_CREAT
 * OM_BOTH      WM_OVERWRITE    O_RDWR|O_TRUNC|O_CREAT
 * OM_BOTH      WM_APPEND       O_RDWR|O_APPEND|O_CREAT
 *
 * As you can see from the table, for FileStream WM_IGNORE and
 * WM_OVERWRITE are equivalent. However, this is only a coincidence and
 * should be treated as undocumented feature and not relied upon.
 *
 * All considerations which can be found in the open(2) man page apply.
 */
class FileStream: public FDStream {
public:
    FileStream(const std::string &filename,
               const OpenMode openmode,
               const WriteMode writemode = WriteMode::IGNORE,
               const ShareMode sharemode = ShareMode::DONT_CARE);
private:
    const OpenMode _openmode;
    bool _seekable;

public:
    virtual bool is_readable() const;
    virtual bool is_seekable() const;
    virtual bool is_writable() const;
};

/**
 * Use the OS API to open a file using the given mode specifiers. For a detailed
 * meaning of the modes with respect to the POSIX API, see the FileStream class.
 */
int open_file_with_modes(const std::string &filename,
                         const OpenMode openmode,
                         const WriteMode writemode,
                         const ShareMode sharemode);


}

#endif
