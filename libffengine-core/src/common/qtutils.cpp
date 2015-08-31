/**********************************************************************
File name: qtutils.cpp
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
#include "ffengine/common/qtutils.hpp"


QtStreamBuf::QtStreamBuf(QIODevice *iodev):
    m_iodev(iodev)
{

}

QtStreamBuf::pos_type QtStreamBuf::seekoff(
        off_type offset,
        std::ios_base::seekdir direction,
        std::ios_base::openmode mode)
{
    pos_type pos = 0;

    switch (direction)
    {
    case std::ios_base::beg:
    {
        pos = offset;
        break;
    }
    case std::ios_base::cur:
    {
        pos = m_iodev->pos() + offset;
        break;
    }
    case std::ios_base::end:
    {
        pos = m_iodev->size() - offset;
        break;
    }
    default:;
    }

    return seekpos(pos, mode);
}

QtStreamBuf::pos_type QtStreamBuf::seekpos(
        pos_type position,
        std::ios_base::openmode)
{
    if (m_iodev->seek(position)) {
        return m_iodev->pos();
    } else {
        return pos_type(off_type(-1));
    }
}

std::streamsize QtStreamBuf::xsgetn(char_type *buf,
                                    std::streamsize n)
{
    return m_iodev->read(buf, n);
}


QtIStream::QtIStream(QIODevice *iodev, bool owned):
    std::istream(m_buf = new QtStreamBuf(iodev)),
    m_iodev(iodev),
    m_owned(owned)
{

}

QtIStream::~QtIStream()
{
    delete m_buf;
    if (m_owned) {
        delete m_iodev;
    }
}
