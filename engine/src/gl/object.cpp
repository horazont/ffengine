/**********************************************************************
File name: object.cpp
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
#include "engine/gl/object.hpp"

#include <GL/glu.h>

#include <cassert>


namespace engine {

_GLObjectBase::_GLObjectBase():
    m_glid(0)
{

}

_GLObjectBase::~_GLObjectBase()
{
    if (m_glid != 0) {
        delete_globject();
    }
}

_GLObjectBase::_GLObjectBase(_GLObjectBase &&ref):
    m_glid(ref.m_glid)
{
    ref.m_glid = 0;
}

_GLObjectBase &_GLObjectBase::operator=(_GLObjectBase &&ref)
{
    if (m_glid != 0) {
        delete_globject();
    }
    m_glid = ref.m_glid;
    return *this;
}

void _GLObjectBase::delete_globject()
{
    m_glid = 0;
}

void _GLObjectBase::bound()
{

}

}
