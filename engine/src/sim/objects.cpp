#include "engine/sim/objects.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>


namespace sim {

const ObjectID NULL_OBJECT_ID = 0;


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
                ObjectIDRegion{1, std::numeric_limits<ObjectID>::max()-1}
                );
}

inline ObjectChunk *ObjectManager::get_object_chunk(ObjectID object_id)
{
    if (object_id == NULL_OBJECT_ID) {
        return nullptr;
    }

    // offset by one, we don’t waste space here
    --object_id;

    const std::vector<ObjectChunk>::size_type chunk_index =
            object_id / ObjectChunk::CHUNK_SIZE;

    if (chunk_index >= m_chunks.size()) {
        return nullptr;
    }

    return &m_chunks[chunk_index];
}

inline std::unique_ptr<Object> *ObjectManager::get_object_ptr(ObjectID object_id)
{
    ObjectChunk *chunk = get_object_chunk(object_id);
    if (!chunk) {
        return nullptr;
    }
    return &(chunk->objects[object_id % ObjectChunk::CHUNK_SIZE]);
}

inline ObjectChunk &ObjectManager::require_object_chunk(ObjectID object_id)
{
    if (object_id == NULL_OBJECT_ID) {
        throw std::runtime_error("NULL_OBJECT require");
    }

    // offset by one, we don’t waste space here
    --object_id;

    const std::vector<ObjectChunk>::size_type chunk_index =
            object_id / ObjectChunk::CHUNK_SIZE;

    if (chunk_index >= m_chunks.size()) {
        m_chunks.resize(chunk_index+1);
    }

    return m_chunks[chunk_index];
}

inline std::unique_ptr<Object> &ObjectManager::require_object_ptr(ObjectID object_id)
{
    return require_object_chunk(object_id).objects[object_id % ObjectChunk::CHUNK_SIZE];
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

    return result;
}

Object *ObjectManager::get_base(ObjectID object_id)
{
    std::unique_ptr<Object> *object_ptr = get_object_ptr(object_id);
    if (!object_ptr) {
        return nullptr;
    }
    return object_ptr->get();
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
    require_object_ptr(obj->object_id()) = std::move(obj);
}

void ObjectManager::kill(ObjectID object_id)
{
    if (object_id == NULL_OBJECT_ID) {
        return;
    }

    std::unique_ptr<Object> *object_ptr = get_object_ptr(object_id);
    if (!object_ptr) {
        return;
    }
    if (!(*object_ptr)) {
        return;
    }

    *object_ptr = nullptr;
    release_object_id(object_id);
}


}
