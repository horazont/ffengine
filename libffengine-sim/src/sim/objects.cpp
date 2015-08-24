/**********************************************************************
File name: objects.cpp
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
#include "ffengine/sim/objects.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <limits>


namespace sim {

constexpr Object::ID NULL_OBJECT_ID = 0;


/* sim::Object */

Object::Object(const ID object_id):
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
                IDRegion{1, std::numeric_limits<Object::ID>::max()-1}
                );
}

inline ObjectManager::Chunk *ObjectManager::get_object_chunk(
        Object::ID object_id)
{
    if (object_id == NULL_OBJECT_ID) {
        return nullptr;
    }

    // offset by one, we don’t waste space here
    --object_id;

    const std::vector<Chunk>::size_type chunk_index =
            object_id / Chunk::CHUNK_SIZE;

    if (chunk_index >= m_chunks.size()) {
        return nullptr;
    }

    return &m_chunks[chunk_index];
}

inline std::unique_ptr<Object> *ObjectManager::get_object_ptr(Object::ID object_id)
{
    Chunk *chunk = get_object_chunk(object_id);
    if (!chunk) {
        return nullptr;
    }
    return &(chunk->objects[(object_id-1) % Chunk::CHUNK_SIZE]);
}

inline ObjectManager::Chunk &ObjectManager::require_object_chunk(Object::ID object_id)
{
    if (object_id == NULL_OBJECT_ID) {
        throw std::runtime_error("NULL_OBJECT require");
    }

    // offset by one, we don’t waste space here
    --object_id;

    const std::vector<Chunk>::size_type chunk_index =
            object_id / Chunk::CHUNK_SIZE;

    if (chunk_index >= m_chunks.size()) {
        m_chunks.resize(chunk_index+1);
    }

    return m_chunks[chunk_index];
}

inline std::unique_ptr<Object> &ObjectManager::require_object_ptr(Object::ID object_id)
{
    return require_object_chunk(object_id).objects[(object_id-1) % Chunk::CHUNK_SIZE];
}

Object::ID ObjectManager::allocate_object_id()
{
    if (m_free_list.empty()) {
        throw std::runtime_error("Out of ObjectIDs. What kind of machine are "
                                 "you using?? These are 64bit integers! You "
                                 "cannot simply exhaust the 64bit integer "
                                 "space without running out of memory first!");
    }
    IDRegion &first = m_free_list.front();
    const Object::ID result = first.first++;
    first.count--;
    if (first.count == 0) {
        m_free_list.erase(m_free_list.begin());
    }

    return result;
}

void ObjectManager::emplace_object(std::unique_ptr<Object> &&obj)
{
    const Object::ID object_id = obj->object_id();

    auto match = std::lower_bound(
                m_free_list.begin(),
                m_free_list.end(),
                object_id,
                [](const IDRegion &region, const Object::ID object_id)
    {
        return (region.first <= object_id);
    });

    if (match == m_free_list.begin()) {
        // the match is above the object_id, but it is the first entry in the
        // free list
        throw std::runtime_error("emplace_object id not in free list");
    }
    --match;

    IDRegion &region = *match;
    // we know that region.first < obj->object_id(), as per std::lower_bound
    assert(region.first <= object_id);

    if (region.first + region.count <= object_id) {
        // the object id is above the region, and the next region is above the
        // object id.
        throw std::runtime_error("emplace_object id not in free list");
    }

    require_object_ptr(object_id) = std::move(obj);

    // first the easy cases:
    if (region.first == object_id) {
        // id is at the beginning of the region
        region.first++;
        region.count--;
        if (region.count == 0) {
            m_free_list.erase(match);
        }
        return;
    } else if (region.first + region.count - 1 == object_id) {
        // id is at the end of the region
        region.count--;
        // otherwise, this must had been handled by the previous if!
        assert(region.count > 0);
        return;
    } else {
        // the general case. I hate the general cases. We have to split the
        // region. For ease of inserting, we use the current region for the
        // upper range and create a new one for the lower range.
        IDRegion new_region;
        new_region.first = region.first;
        new_region.count = object_id - region.first;

        region.first = object_id + 1;
        region.count = region.count - (new_region.count + 1);

        // take care to emplace into the free_list at the end, otherwise the
        // region reference invalidates.
        m_free_list.emplace(match, new_region);
    }
}

Object *ObjectManager::get_base(Object::ID object_id)
{
    std::unique_ptr<Object> *object_ptr = get_object_ptr(object_id);
    if (!object_ptr) {
        return nullptr;
    }
    return object_ptr->get();
}

void ObjectManager::release_object_id(Object::ID object_id)
{
    auto match = std::lower_bound(
                m_free_list.begin(),
                m_free_list.end(),
                object_id,
                [](const IDRegion &region, const Object::ID object_id)
    {
        return (region.first < object_id);
    });

    if (match != m_free_list.begin()) {
        auto previous = match-1;
        IDRegion &region = *previous;
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

    m_free_list.insert(match, IDRegion{object_id, 1});
}

void ObjectManager::set_object(std::unique_ptr<Object> &&obj)
{
    require_object_ptr(obj->object_id()) = std::move(obj);
}

void ObjectManager::kill(Object::ID object_id)
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

std::ostream &ObjectManager::dump_free_list(std::ostream &out)
{
    unsigned int i = 0;
    for (auto &region: m_free_list)
    {
        out << "entry " << std::setfill(' ') << std::setw(3) << i << std::endl;
        out << "  first = 0x"
            << std::setbase(16) << std::setw(16) << std::setfill('0') << region.first
            << "  count = 0x"
            << std::setbase(16) << std::setw(16) << region.count
            << std::endl;
        ++i;
    }
    return out;
}


}
