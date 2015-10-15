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
#ifndef SCC_ENGINE_RESOURCE_H
#define SCC_ENGINE_RESOURCE_H

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>


namespace ffe {

class ResourceManager;

class Resource
{
public:
    virtual ~Resource();

private:
    ResourceManager *m_manager;
    std::string m_name;

public:
    inline ResourceManager *manager() const {
        return m_manager;
    }

    inline const std::string &name() const {
        return m_name;
    }

    friend class ResourceManager;

};

/**
 * @brief The ResourceManager class is the PID 1 of resources.
 */
class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

private:
    std::vector<std::unique_ptr<Resource> > m_resources;
    std::unordered_map<std::string, Resource*> m_resource_map;

protected:
    void insert_resource_unchecked(const std::string &name,
                                   std::unique_ptr<Resource> &&res);
    void require_unused_name(const std::string &name);

public:
    template <typename T, typename... args_ts>
    T &emplace(const std::string &name, args_ts&&... args)
    {
        require_unused_name(name);

        T *obj = new T(std::forward<args_ts>(args)...);
        insert_resource_unchecked(name, std::unique_ptr<Resource>(obj));
        return *obj;
    }


    Resource *get(const std::string &name);

    template <typename T>
    T *get_safe(const std::string &name)
    {
        Resource *tmp = get(name);
        return dynamic_cast<T*>(tmp);
    }

    template <typename T>
    T &manage(const std::string &name,
                     std::unique_ptr<T> &&res)
    {
        require_unused_name(name);

        T *res_ptr = res.get();
        insert_resource_unchecked(name, std::move(res));
        return *res_ptr;
    }

    void release(const std::string &name);

};

}

#endif
