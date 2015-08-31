/**********************************************************************
File name: resource.hpp
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
#ifndef SCC_GL_RESOURCE_H
#define SCC_GL_RESOURCE_H

#include <spp/spp.hpp>

#include "ffengine/common/resource.hpp"

namespace engine {


class QFileLoader: public spp::Loader
{
public:
    std::unique_ptr<std::istream> open(const std::string &path);
};



class GLResourceManager: public ResourceManager
{
public:
    GLResourceManager();

private:
    spp::Library m_library;

public:
    inline spp::Library &shader_library()
    {
        return m_library;
    }

    inline const spp::Library &shader_library() const
    {
        return m_library;
    }

    const spp::Program &load_shader_checked(const std::string &path);

};

}

#endif
