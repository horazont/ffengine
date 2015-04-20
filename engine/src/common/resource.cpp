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
#include "engine/common/resource.hpp"


namespace engine {

Resource::~Resource()
{

}


ResourceManager::ResourceManager():
    m_resources(),
    m_resource_map()
{

}

ResourceManager::~ResourceManager()
{
    m_resource_map.clear();
    // force deletion of resources in reverse registration order
    for (auto iter = m_resources.rbegin();
         iter != m_resources.rend();
         ++iter)
    {
        iter->reset();
    }
}

void ResourceManager::insert_resource_unchecked(
        const std::string &name,
        std::unique_ptr<Resource> &&res)
{
    res->m_name = name;
    m_resource_map[name] = res.get();
    m_resources.emplace_back(std::move(res));
}

void ResourceManager::require_unused_name(const std::string &name)
{
    auto iter = m_resource_map.find(name);
    if (iter != m_resource_map.end()) {
        throw std::runtime_error("duplicate resource name: "+name);
    }
}

Resource *ResourceManager::get(const std::string &name)
{
    auto iter = m_resource_map.find(name);
    if (iter == m_resource_map.end()) {
        return nullptr;
    }

    return iter->second;
}

void ResourceManager::release(const std::string &name)
{
    auto iter = m_resource_map.find(name);
    if (iter == m_resource_map.end()) {
        return;
    }

    m_resource_map.erase(iter);
}

}
