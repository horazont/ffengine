/**********************************************************************
File name: qtutils.hpp
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
#ifndef SCC_COMMON_QTUTILS_H
#define SCC_COMMON_QTUTILS_H

#include <QIODevice>

class QtStreamBuf: public std::streambuf
{
public:
    QtStreamBuf(QIODevice *iodev);

private:
    static constexpr std::streamsize BUFFER_SIZE = 1024;

    QIODevice *m_iodev;
    char_type m_inbuf[BUFFER_SIZE];

protected:
    pos_type seekoff(off_type offset, std::ios_base::seekdir direction, std::ios_base::openmode mode) override;
    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override;
    std::streamsize xsgetn(char_type *buf, std::streamsize n) override;

};


class QtIStream: public std::istream
{
public:
    QtIStream(QIODevice *iodev, bool owned = true);
    ~QtIStream() override;

private:
    QtStreamBuf *m_buf;
    QIODevice *m_iodev;
    bool m_owned;

};

#endif
