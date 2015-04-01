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
