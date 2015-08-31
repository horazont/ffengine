/**********************************************************************
File name: resource.cpp
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
#include "ffengine/gl/resource.hpp"

#include <QFile>

#include "ffengine/common/qtutils.hpp"
#include "ffengine/io/log.hpp"


namespace engine {

static io::Logger &logger = io::logging().get_logger("gl.resource");

GLResourceManager::GLResourceManager():
    m_library(std::make_unique<QFileLoader>())
{

}

const spp::Program &GLResourceManager::load_shader_checked(const std::string &path)
{
    const spp::Program *prog = m_library.load(path);
    if (!prog) {
        logger.logf(io::LOG_ERROR, "failed to load shader from %s", path.c_str());
        throw std::runtime_error("failed to load shader template");
    }

    if (!prog->errors().empty()) {
        for (auto &error: prog->errors()) {
            logger.logf(io::LOG_ERROR, "%s:%d:%d: %s",
                        std::get<0>(error).c_str(),
                        std::get<1>(error).begin.line,
                        std::get<1>(error).begin.column,
                        std::get<2>(error).c_str());
        }
        throw std::runtime_error("failed to load shader template");
    }

    return *prog;
}

std::unique_ptr<std::istream> engine::QFileLoader::open(const std::string &path)
{
    QFile source_file(QString::fromStdString(path));
    if (!source_file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return nullptr;
    }
    QByteArray data = source_file.readAll();
    data.append("\0");
    return std::make_unique<std::istringstream>(std::string(data.data(), data.size()));
}

}
