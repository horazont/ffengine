/**********************************************************************
File name: objects.hpp
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
#ifndef SCC_SIM_OBJECTS_H
#define SCC_SIM_OBJECTS_H

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

namespace sim {

typedef std::uint64_t ObjectID;
extern const ObjectID NULL_OBJECT_ID;

class ObjectManager;


/**
 * A network addressable object.
 *
 * Each object has its own unique \a object_id. No two objects with the same
 * object id exist at the same time within the same ObjectManager.
 *
 * To delete an object, do not call delete. Use the ObjectManager::kill()
 * method.
 */
class Object
{
public:
    /**
     * Create a new Object with the given ObjectID.
     *
     * Generally, it is not advisable to create objects this way. Use
     * ObjectManager::allocate() instead, which will also allocate the
     * ObjectID for you.
     *
     * @param object_id ObjectID as allocated by an ObjectManager.
     */
    explicit Object(const ObjectID object_id);
    virtual ~Object();

private:
    ObjectID m_object_id;

public:
    /**
     * ObjectID of this object.
     */
    inline ObjectID object_id() const
    {
        return m_object_id;
    }

};


/**
 * Bookkeeping structure for keeping track of unused ID chunks in
 * ObjectManager.
 */
struct ObjectIDRegion
{
    /**
     * First ObjectID contained in the region.
     */
    ObjectID first;

    /**
     * Amount of ObjectIDs covered by the region. 0 is not a valid value.
     */
    ObjectID count;
};


/**
 * A chunk of object pointers. The chunk size must be a power of two. Chunks
 * are allocated at once and filled with objects as needed.
 */
struct ObjectChunk
{
    static constexpr std::size_t CHUNK_SIZE = 4096;

    std::array<std::unique_ptr<Object>, CHUNK_SIZE> objects;
};


/**
 * A manager to keep an association between object IDs and objects.
 *
 * The manager also owns the Object instances. All game objects which need to
 * be addressable over the network must be derived from Object.
 */
class ObjectManager
{
public:
    ObjectManager();

private:
    std::vector<ObjectChunk> m_chunks;
    std::vector<ObjectIDRegion> m_free_list;

protected:
    /**
     * Allocate an unused ObjectID.
     *
     * This always returns a valid ID, or does not return at all. As we are
     * using 64 bit IDs, it is unlikely that a game will *ever* run out of
     * IDs before running out of memory.
     *
     * @return An unused ObjectID.
     */
    ObjectID allocate_object_id();

    /**
     * Return a pointer to the Object associated with a given ObjectID.
     *
     * @param object_id ID to look up
     * @return Pointer to the Object associated with \a object_id. If no such
     * object exists, return nullptr.
     */
    Object *get_base(ObjectID object_id);

    /**
     * Release a previously used ObjectID.
     *
     * Calling this with an ObjectID which is currently in the free list
     * *will* corrupt the free list.
     *
     * This is only used in the exception handling code in allocate().
     *
     * @param object_id ID to release
     */
    void release_object_id(ObjectID object_id);

    /**
     * Associate and store an Object with an ObjectID.
     *
     * @param obj Object to store. The ObjectID is obtained from the object.
     */
    void set_object(std::unique_ptr<Object> &&obj);

public:
    /**
     * Allocate a new object of type \a T.
     *
     * The arguments are passed to the constructor of \a T, prepended with the
     * new objects ID.
     */
    template <typename T, typename... arg_ts>
    T &allocate(arg_ts... args)
    {
        const ObjectID object_id = allocate_object_id();
        T *obj = nullptr;
        std::unique_ptr<T> instance;
        try {
            instance = std::make_unique<T>(
                        object_id,
                        std::forward<arg_ts>(args)...);
            obj = instance.get();
        } catch (...) {
            // release the object id
            release_object_id(object_id);
            throw;
        }
        set_object(std::move(instance));
        return *obj;
    }

    /**
     * Return the object identified by \a object_id with type checking.
     *
     * The object stored at the given ID is dynamically casted to \a T.
     *
     * @param object_id ID of the object to retrieve
     * @return The object associated with \a object_id, if any, dynamically
     * casted to \a T. The return value may be nullptr, if no such object
     * exists or the object is of a different type.
     */
    template <typename T>
    T *get_safe(ObjectID object_id)
    {
        return dynamic_cast<T*>(get_base(object_id));
    }

    /**
     * Return the object identified by \a object_id without type checking.
     *
     * The object stored at the given ID is statically casted to \a T. The
     * usual implications apply if the stored object is not compatible to type
     * \a T.
     *
     * @param object_id ID of the object to retrieve
     * @return The object associated with \a object_id, if any, dynamically
     * casted to \a T. The return value may be nullptr, if no such object
     * exists.
     */
    template <typename T>
    T *get_unsafe(ObjectID object_id)
    {
        return static_cast<T*>(get_base(object_id));
    }

    /**
     * Delete an object using its object id.
     *
     * The object is deleted immediately. If no object is associated with the
     * given ID, this is a no-op.
     *
     * @param object_id The ID of the object to delete.
     */
    void kill(ObjectID object_id);

};


}

#endif
