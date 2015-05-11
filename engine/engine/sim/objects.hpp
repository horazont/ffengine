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
     * ID type to refer to objects.
     *
     * Object IDs are managed by ObjectManager instances.
     */
    typedef std::uint64_t ID;

    /**
     * The ID to refer to a nonexistant object. The value of this constant is
     * implementation-defined and should not be relied upon.
     */
    static constexpr ID NULL_OBJECT_ID = 0;

public:
    /**
     * Create a new Object with the given Object::ID.
     *
     * Generally, it is not advisable to create objects this way. Use
     * ObjectManager::allocate() instead, which will also allocate the
     * Object::ID for you.
     *
     * @param object_id Object::ID as allocated by an ObjectManager.
     */
    explicit Object(const ID object_id);
    virtual ~Object();

private:
    ID m_object_id;

public:
    /**
     * Object::ID of this object.
     */
    inline ID object_id() const
    {
        return m_object_id;
    }

};


/**
 * A manager to keep an association between object IDs and objects.
 *
 * The manager also owns the Object instances. All game objects which need to
 * be addressable over the network must be derived from Object.
 *
 * The ObjectManager is not thread-safe.
 */
class ObjectManager
{
private:
    /**
     * A chunk of object pointers. Chunks are allocated at once and filled with
     * objects as needed.
     */
    struct Chunk
    {
        /**
         * Number of objects in a single chunk.
         *
         * This number is chosen arbitrarily. It should be a power of two, although
         * this is not a strict requirement. It is probably helpful if the size of
         * ObjectChunk matches the page size (which it doesn’t, on AMD64,
         * currently).
         */
        static constexpr std::size_t CHUNK_SIZE = 4096;

        /**
         * Array of objects in this chunk.
         *
         * The array can be indexed using the IDs. The addressing of objects
         * within chunks is implementation-defined by the ObjectManager.
         */
        std::array<std::unique_ptr<Object>, CHUNK_SIZE> objects;
    };

    /**
     * Bookkeeping structure for keeping track of unused ID chunks in
     * ObjectManager.
     */
    struct IDRegion
    {
        /**
         * First Object::ID contained in the region.
         */
        Object::ID first;

        /**
         * Amount of object ids covered by the region. 0 is not a valid value.
         */
        Object::ID count;
    };

public:
    ObjectManager();

private:
    std::vector<Chunk> m_chunks;
    std::vector<IDRegion> m_free_list;

private:
    /**
     * Return a pointer to the ObjectChunk which holds the given Object::ID.
     *
     * If the addressed Object::ID is out of the currently reserved range,
     * nullptr is returned. If the \a object_id is the NULL_OBJECT_ID, nullptr
     * is returned.
     *
     * @param object_id Object::ID to look up
     * @return Pointer to the ObjectChunk or nullptr.
     */
    Chunk *get_object_chunk(Object::ID object_id);

    /**
     * Return the a pointer to the std::unique_ptr for the object with the
     * given Object::ID.
     *
     * If the Object::ID \a object_id is NULL_OBJECT_ID, nullptr is returned.
     * Otherwise, a pointer to a valid std::unique_ptr instance is returned.
     * That instance may itself be a nullptr, if no object is currently
     * associated with the given object id.
     *
     * @param object_id Object::ID to look up
     * @return Pointer to an std::unique_ptr object for the object or nullptr.
     */
    std::unique_ptr<Object> *get_object_ptr(Object::ID object_id);

    /**
     * Return a reference to the ObjectChunk which holds the given Object::ID.
     *
     * If the addressed Object::ID is out of the currently reserved range,
     * the range is extended accordingly. If \a object_id is NULL_OBJECT_ID,
     * std::runtime_error is thrown.
     *
     * @param object_id Object::ID to look up
     * @return Pointer to the ObjectChunk or nullptr.
     * @throws std::runtime_error if \a object_id is NULL_OBJECT_ID.
     */
    Chunk &require_object_chunk(Object::ID object_id);

    /**
     * Return the a reference to the std::unique_ptr for the object with the
     * given Object::ID.
     *
     * If the Object::ID is out of the currently reserved range, the range is
     * extended to cover the Object::ID. This results in a possibly large
     * reallocation.
     *
     * If the Object::ID is the NULL_OBJECT_ID, std::runtime_error is thrown.
     *
     * @param object_id Object::ID to look up
     * @return Pointer to an std::unique_ptr object for the object or nullptr.
     * @throws std::runtime_error if the Object::ID is NULL_OBJECT_ID.
     */
    std::unique_ptr<Object> &require_object_ptr(Object::ID object_id);

protected:
    /**
     * Allocate an unused Object::ID.
     *
     * This always returns a valid ID, or does not return at all. As we are
     * using 64 bit IDs, it is unlikely that a game will *ever* run out of
     * IDs before running out of memory.
     *
     * @return An unused Object::ID.
     */
    Object::ID allocate_object_id();

    /**
     * Return a pointer to the Object associated with a given Object::ID.
     *
     * @param object_id ID to look up
     * @return Pointer to the Object associated with \a object_id. If no such
     * object exists, return nullptr.
     */
    Object *get_base(Object::ID object_id);

    /**
     * Release a previously used Object::ID.
     *
     * Calling this with an Object::ID which is currently in the free list
     * *will* corrupt the free list.
     *
     * This is only used in the exception handling code in allocate().
     *
     * @param object_id ID to release
     */
    void release_object_id(Object::ID object_id);

    /**
     * Associate and store an Object with an Object::ID.
     *
     * @param obj Object to store. The Object::ID is obtained from the object.
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
        const Object::ID object_id = allocate_object_id();
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
    T *get_safe(Object::ID object_id)
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
    T *get_unsafe(Object::ID object_id)
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
    void kill(Object::ID object_id);

};


}

#endif
