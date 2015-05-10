#include "engine/sim/objects.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>


namespace sim {

const ObjectID NULL_OBJECT_ID = std::numeric_limits<ObjectID>::max();


/* sim::Object */

Object::Object(const ObjectID object_id):
    m_object_id(object_id)
{

}

Object::~Object()
{

}


/* sim::ObjectManager */

ObjectManager::ObjectManager()
{
    m_free_list.emplace_back(
                ObjectIDRegion{0, std::numeric_limits<ObjectID>::max()-1}
                );
}

ObjectID ObjectManager::allocate_object_id()
{
    if (m_free_list.empty()) {
        throw std::runtime_error("Out of ObjectIDs. What kind of machine are "
                                 "you using?? These are 64bit integers! You "
                                 "cannot simply exhaust the 64bit integer "
                                 "space without running out of memory first!");
    }
    ObjectIDRegion &first = m_free_list.front();
    const ObjectID result = first.first++;
    first.count--;
    if (first.count == 0) {
        m_free_list.erase(m_free_list.begin());
    }

    m_chunks.resize(std::max((result / ObjectChunk::CHUNK_SIZE)+1,
                             m_chunks.size()));

    return result;
}

Object *ObjectManager::get_base(ObjectID object_id)
{
    const std::vector<ObjectChunk>::size_type chunk_index = object_id / ObjectChunk::CHUNK_SIZE;
    if (chunk_index >= m_chunks.size()) {
        return nullptr;
    }
    return m_chunks[chunk_index].objects[object_id % ObjectChunk::CHUNK_SIZE].get();
}

void ObjectManager::release_object_id(ObjectID object_id)
{
    auto match = std::lower_bound(
                m_free_list.begin(),
                m_free_list.end(),
                object_id,
                [](const ObjectIDRegion &region, const ObjectID object_id)
    {
        return (region.first < object_id);
    });

    if (match != m_free_list.begin()) {
        auto previous = match-1;
        ObjectIDRegion &region = *previous;
        if (region.first > 0 && region.first - 1 == object_id) {
            region.count++;
            region.first--;
            // check if there is another previous region
            if (previous != m_free_list.begin()) {
                // try to merge
                auto preprevious = previous-1;
                if (preprevious->first + preprevious->count == region.first) {
                    // we can merge
                    preprevious->count += region.count;
                    m_free_list.erase(previous);
                }
            }
            return;
        } else if (region.first + region.count == object_id) {
            region.count++;
            // check if the next region is now adjacent to the current
            if (match->first == region.first + region.count) {
                // merge with next
                region.count += match->count;
                m_free_list.erase(match);
            }
            return;
        }
    }

    m_free_list.insert(match, ObjectIDRegion{object_id, 1});
}

void ObjectManager::set_object(std::unique_ptr<Object> &&obj)
{
    const ObjectID object_id = obj->object_id();
    const std::vector<ObjectChunk>::size_type chunk_index = object_id / ObjectChunk::CHUNK_SIZE;
    m_chunks[chunk_index].objects[object_id % ObjectChunk::CHUNK_SIZE] = std::move(obj);
}

void ObjectManager::kill(ObjectID object_id)
{
    const std::vector<ObjectChunk>::size_type chunk_index = object_id / ObjectChunk::CHUNK_SIZE;
    if (chunk_index >= m_chunks.size()) {
        return;
    }
    /* std::cout << "deleting object id " << object_id << std::endl;*/
    std::unique_ptr<Object> &obj = m_chunks[chunk_index].objects[object_id % ObjectChunk::CHUNK_SIZE];
    if (!obj) {
        return;
    }
    obj = nullptr;

    release_object_id(object_id);
}


}
