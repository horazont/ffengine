/**********************************************************************
File name: utils.cpp
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
#include "ffengine/io/utils.hpp"

#include <vector>
#include <stdexcept>

namespace io {

std::string absolutify(const std::string &path)
{
    std::vector<std::string> items;
    std::string::size_type prev = 0;
    for (std::string::size_type i = 0; i < path.size(); i++) {
        if (path[i] == '/') {
            items.push_back(
                path.substr(prev, i-prev));
            prev = i+1;
        }
    }
    items.push_back(path.substr(prev, path.size()-prev));

    if (items[0].empty()) {
        items.erase(items.begin());
    }

    std::vector<std::string>::size_type i = 0;
    while (i < items.size()) {
        std::string &segment = items[i];
        if (segment.empty()) {
            i++;
            continue;
        }
        if (segment == ".") {
            items.erase(items.begin()+i);
            continue;
        }
        if (segment == "..") {
            if (i > 0) {
                items.erase(items.begin()+i);
                i--;
                items.erase(items.begin()+i);
                continue;
            } else {
                throw std::invalid_argument(
                    "Relative path leaves root scope.");
            }
        }
        i++;
    }

    std::string result = "";
    for (auto &segment: items) {
        result += "/" + segment;
    }
    return result;
}

std::string basename(const std::string &path)
{
    std::string::size_type pos = path.rfind("/");
    if (pos == std::string::npos) {
        return path;
    }
    return path.substr(pos+1);
}

std::string dirname(const std::string &path)
{
    std::string::size_type pos = path.rfind("/");
    if (pos == std::string::npos) {
        return "";
    }
    return path.substr(0, pos);
}

std::string join(const std::initializer_list<std::string> &segments)
{
    std::string full_path = "";
    for (auto &segment: segments) {
        if (segment.empty()) {
            continue;
        }
        std::string abssegment = absolutify(segment);
        if (segment[0] == '/') {
            full_path = abssegment;
        } else {
            if (full_path.empty()) {
                full_path = abssegment.substr(1, abssegment.size()-1);
            } else {
                full_path += abssegment;
            }
        }
    }
    return full_path;
}

std::string normalize_vfs_path(const std::string &path)
{
    if (!path.empty() && (path[path.size()-1] == '/')) {
        return path.substr(0, path.size()-1);
    }
    return path;
}

std::pair<std::string, std::string> splitext(
    const std::string &fullpath)
{
    std::string::size_type pos = fullpath.rfind('/');
    std::string path, filename;
    if (pos == std::string::npos) {
        path = "";
        filename = fullpath;
    } else {
        path = fullpath.substr(0, pos);
        filename = fullpath.substr(pos+1);
    }

    std::string basename, ext;
    pos = filename.rfind('.');
    if (pos == std::string::npos) {
        basename = filename;
        ext = "";
    } else {
        basename = filename.substr(0, pos);
        ext = filename.substr(pos+1);
    }

    return std::make_pair(path + "/" + basename, ext);
}

void validate_vfs_path(const std::string &path)
{
    if (path != absolutify(path)) {
        throw std::invalid_argument("Invalid VFS path: `"+path+"': "
            "VFS paths must be absolute and must not contain double "
            "slashes.");
    }
}

}

