/**********************************************************************
File name: stdiostream.hpp
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
