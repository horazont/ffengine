/**********************************************************************
File name: stream.cpp
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
#include "ffengine/io/stream.hpp"

#include <cstring>
#include <typeinfo>
#include <limits>
#include <iostream>

namespace io {

/* io::Stream */

void Stream::flush()
{

}

std::size_t Stream::read(void *, const std::size_t)
{
    throw StreamNotSupportedError(std::string(typeid(this).name()) + " does not support reading");
}

std::size_t Stream::read(char *data, const std::size_t length)
{
    return read((void*)data, length);
}

std::size_t Stream::seek(const int, const std::ptrdiff_t)
{
    raise_seek_not_supported_error();
    return 0;
}

std::size_t Stream::size() const
{
    raise_seek_not_supported_error();
    return 0;
}

std::size_t Stream::tell() const
{
    return 0;
}

std::size_t Stream::write(const void *, const std::size_t)
{
    throw StreamNotSupportedError(std::string(typeid(this).name()) + " does not support writing");
}

std::size_t Stream::write(const char *data, const std::size_t length)
{
    return write((const void*)data, length);
}

void Stream::close()
{

}

void Stream::raise_seek_not_supported_error() const
{
    throw StreamNotSupportedError(std::string(typeid(this).name()) + " does not support seeking");
}

void Stream::raise_read_error(const std::size_t read, const std::size_t required) const
{
    throw StreamNotSupportedError("Read error: "+std::to_string(read)+" out of "+std::to_string(required)+" bytes read");
}

void Stream::raise_write_error(const std::size_t written, const std::size_t required) const
{
    throw StreamNotSupportedError("Write error: "+std::to_string(written)+" out of "+std::to_string(required)+" bytes written");
}

template <class _T> _T Stream::read_int()
{
    _T result;
    const std::size_t readBytes = read(&result, sizeof(_T));
    if (readBytes < sizeof(_T)) {
        raise_read_error(readBytes, sizeof(_T));
    }
    return result;
}

template <class _T> void Stream::write_int(const _T value)
{
    const std::size_t writtenBytes = write(&value, sizeof(_T));
    if (writtenBytes < sizeof(_T)) {
        raise_write_error(writtenBytes, sizeof(_T));
    }
}

void Stream::read_bytes(void *data, const std::size_t length)
{
    const std::size_t readBytes = read(data, length);
    if (readBytes < length) {
        raise_read_error(readBytes, length);
    }
}

std::string Stream::read_string(const std::size_t length)
{
    void *buffer = malloc(length);
    try {
        read_bytes(buffer, length);
    } catch (...) {
        free(buffer);
        throw;
    }
    std::string result((char*)buffer);
    free(buffer);
    return result;
}

void Stream::write_endl()
{
    #if defined (__linux__)
        static const int length = 1;
        static const char lineEnding[length+1] = "\n";
    #else
        #if defined (__win32__)
            static const int length = 2;
            static const char lineEnding[length+1] = "\r\n";
        #else
            #if defined (__APPLE__)
                static const int length = 1;
                static const char lineEnding[length+1] = "\r";
            #else
                static_assert(false, "Could not detect operating system.");
            #endif
        #endif
    #endif
    write(lineEnding, length);
}

std::basic_string<uint8_t> Stream::read_all(std::ptrdiff_t block_size)
{
    std::basic_string<uint8_t> result;
    if (is_seekable()) {
        // figure out the length and read in one run
        std::size_t old_pos = tell();
        std::size_t end_pos = seek(SEEK_END, 0);
        seek(SEEK_SET, old_pos);
        std::size_t total = end_pos - old_pos;
        result.resize(total);
        std::ptrdiff_t read_bytes = read(&result.front(), total);
        result.resize(read_bytes);
        return result;
    }

    std::ptrdiff_t read_bytes;
    std::basic_string<uint8_t> buffer(block_size, 0);
    do {
        read_bytes = read(&buffer.front(), buffer.size());
        buffer.resize(read_bytes);
        result += buffer;
    } while (read_bytes == block_size);

    return result;
}

#include "stream_operators.inc.cpp"

}
