#ifndef SCC_ENGINE_IO_STREAM_H
#define SCC_ENGINE_IO_STREAM_H

#include <cstdint>
#include <stdexcept>

namespace io {

class StreamError: public std::runtime_error {
public:
    StreamError(const std::string message):
        std::runtime_error(message) {};
    StreamError(const char *message):
        std::runtime_error(message) {};
    virtual ~StreamError() throw() {};

};

class StreamNotSupportedError: public StreamError {
public:
    StreamNotSupportedError(const std::string message):
        StreamError(message) {};
    StreamNotSupportedError(const char *message):
        StreamError(message) {};
    virtual ~StreamNotSupportedError() throw() {};

};

class StreamReadError: public StreamError {
public:
    StreamReadError(const std::string message):
        StreamError(message) {};
    StreamReadError(const char *message):
        StreamError(message) {};
    virtual ~StreamReadError() throw() {};

};

class StreamWriteError: public StreamError {
public:
    StreamWriteError(const std::string message):
        StreamError(message) {};
    StreamWriteError(const char *message):
        StreamError(message) {};
    virtual ~StreamWriteError() throw() {};

};

/**
 * Class to replace the std::[io]?[f]?stream classes. The rationale for
 * that can be... uh... requested at some of the developers.
 *
 * These streams are binary only. If you want to write human-readable
 * data, you are supposed to format it to a string and use the write
 * string operator.
 *
 * However, we do not provide a >> operator for strings, as any
 * behaviour we could supply would not be satisfactory for all purposes.
 * If you want to read strings, you should use the supplied methods on
 * your own.
 */
class Stream {
public:
    /**
     * Make sure the stream is synchronized with any low-level,
     * hardware or file system primitives. The meaning of this call
     * is dependent on the actual stream type.
     *
     * A flush call should generally be made before switching
     * from reading to writing and vice versa or when its neccessary
     * to be sure that data is actually stored (or sent, if its a
     * network stream for example).
     */
    virtual void flush();

    /**
     * Attempts to read length bytes from the stream and stores
     * these in data. Returns the amount of bytes read. This might
     * be less than length if an error occured. The contents of
     * data behind the last position where data was written to
     * should be considered as uninitialized.
     *
     * This may throw an exception if it is not possible to read
     * from the stream.
     *
     * You may check for reading ability by calling isReadable.
     */
    std::size_t read(char *data, const std::size_t length);

    virtual std::size_t read(void *data, const std::size_t length);

    /**
     * Change the read/write pointer position of the stream. For
     * documentation of whence and offset, see lseek(2).
     *
     * This may throw an exception if it is not possible to seek
     * in the stream.
     *
     * You may check for seeking ability by calling isSeekable.
     */
    virtual std::size_t seek(const int whence, const std::ptrdiff_t offset);

    /**
     * Returns the size of the stream in bytes. This may throw an
     * exception if seeking is not supported. You can check for
     * seek support by calling isSeekable.
     *
     * A similar effect can be achived by calling seek(SEEK_END, 0),
     * but this does not change the current reading/writing
     * position.
     */
    virtual std::size_t size() const;

    /**
     * This has the same effect as seek(SEEK_CUR, 0), except that it
     * should also work on non-seekable streams and constant
     * references.
     *
     * Return value: absolute position of the read/write pointer
     * from the beginning of the stream.
     *
     * You may check for seeking/telling ability by calling
     * isSeekable. If a stream is seekable, it must be able to tell
     * the current position.
     *
     * tell must never throw an exception. At may return 0 if
     * telling the position/bytecount is not supported.
     */
    virtual std::size_t tell() const;

    /**
     * Write length bytes read from data to the stream. Returns the
     * amount of bytes actually written which might be less than
     * length in case of an error.
     *
     * This may throw an exception if its not possible to write to
     * the stream.
     *
     * You may check for writing ability by calling isWritable.
     */
    virtual std::size_t write(const void *data, const std::size_t length);
    std::size_t write(const char *data, const std::size_t length);

    /**
     * Close the stream.
     */
    virtual void close() = 0;
protected:
    void raise_seek_not_supported_error() const;
    void raise_read_error(const std::size_t read, const std::size_t required) const;
    void raise_write_error(const std::size_t written, const std::size_t required) const;
    template <class _T> _T read_int();
    template <class _T> void write_int(const _T value);
public:
    template <class T>
    inline T read_raw() {
        T val;
        read_bytes(&val, sizeof(T));
	    return val;
	}
    void read_bytes(void *data, const std::size_t length);
    std::string read_string(const std::size_t length);

    /**
     * a convenience function to overcome the need to write an \n
     * or \r or both to a char buffer. Writes the current OS:s line
     * ending to the stream.
     */
    void write_endl();

    template <class _T>
    inline void write_raw(const _T value) {
        const std::size_t length = sizeof(_T);
        std::size_t writtenBytes = write(&value, length);
	    if (writtenBytes < length) {
        raise_write_error(writtenBytes, length);
	    }
    }

    std::basic_string<uint8_t> read_all(std::ptrdiff_t block_size = 4096);

public:
    virtual bool is_readable() const = 0;
    virtual bool is_seekable() const = 0;
    virtual bool is_writable() const = 0;
};

#include "stream_operators.inc.hpp"

}

#endif
