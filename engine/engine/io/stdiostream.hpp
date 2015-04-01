#ifndef SCC_ENGINE_IO_STDIOSTREAM_H
#define SCC_ENGINE_IO_STDIOSTREAM_H

#include <memory>

#include "engine/io/filestream.hpp"

namespace io {

class StdIOStream: public FDStream {
public:
    StdIOStream(int origFD);

public:
    virtual bool is_seekable() const;
};

class StdInStream: public StdIOStream {
public:
    StdInStream();

public:
    virtual bool is_readable() const;
    virtual bool is_writable() const;
};

class StdOutStream: public StdIOStream {
public:
    StdOutStream();

public:
    virtual bool is_readable() const;
    virtual bool is_writable() const;
};

class StdErrStream: public StdIOStream {
public:
    StdErrStream();

public:
    virtual bool is_readable() const;
    virtual bool is_writable() const;
};

extern std::unique_ptr<Stream> stdin, stdout, stderr;

}

#endif
