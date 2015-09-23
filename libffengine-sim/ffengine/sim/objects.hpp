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
#include <cassert>
#include <cstdint>
#include <list>
#include <memory>
#include <ostream>
#include <unordered_set>
#include <vector>


namespace sim {

class ObjectManager;

class abstract_object_ptr;

template <typename T>
class object_ptr;

/**
 * A network addressable object.
 *
 * Each object has its own unique \a object_id. No two objects with the same
 * object id exist at the same time within the same ObjectManager.
 *
 * To create Object (or dervied) instances, use ObjectManager::allocate() or
 * ObjectManager::emplace(). To delete Object instances, use
 * ObjectManager::kill(). Directly deleting an Object will bring an army of
 * rampaging dragons upon you.
 *
 * Generally, client and server share the same view of the object IDs, as
 * the server dictates which IDs to use for a new object and clients have
 * no word to say here. This allows clients and servers to refer to objects
 * in messages using the IDs.
 *
 * @see ObjectManager::allocate()
 * @see ObjectManager::emplace()
 * @see ObjectManager::kill()
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
     * Create a new Object with the given ID.
     *
     * Generally, it is not advisable to create objects this way. Use
     * ObjectManager::allocate() instead, which will also allocate the
     * ID for you. If you already have an object ID, use
     * ObjectManager::emplace().
     *
     * @param object_id Object::ID as allocated by an ObjectManager.
     *
     * @see ObjectManager::allocate()
     * @see ObjectManager::emplace()
     */
    explicit Object(const ID object_id);

    /**
     * Delete the Object.
     *
     * Generally, deleting an Object (or subclass) directly is not supported.
     * Use ObjectManager::kill().
     *
     * @see ObjectManager::kill()
     */
    virtual ~Object();

private:
    ID m_object_id;
    std::unordered_set<abstract_object_ptr*> m_referees;

public:
    /**
     * ID of this object.
     *
     * Note that object IDs are scoped within ObjectManager instances and have
     * no meaning outside that scope.
     */
    inline ID object_id() const
    {
        return m_object_id;
    }

    template <typename T> friend class object_ptr;
};


/**
 * Implementation detail
 */
class abstract_object_ptr
{
public:
    explicit abstract_object_ptr(std::nullptr_t = nullptr):
        m_object(nullptr)
    {

    }

    explicit abstract_object_ptr(Object &obj):
        m_object(&obj)
    {

    }

    abstract_object_ptr(const abstract_object_ptr &ref) = default;
    abstract_object_ptr& operator=(const abstract_object_ptr &ref) = default;

    abstract_object_ptr(abstract_object_ptr &&src):
        m_object(src.m_object)
    {
        src.m_object = nullptr;
    }

    abstract_object_ptr &operator=(abstract_object_ptr &&src)
    {
        m_object = src.m_object;
        src.m_object = nullptr;
        return *this;
    }

    abstract_object_ptr &operator=(std::nullptr_t)
    {
        m_object = nullptr;
        return *this;
    }

protected:
    Object *m_object;

    void kill()
    {
        m_object = nullptr;
    }

    friend class Object;
};


/**
 * The object_ptr is used to have a special type of weak pointer to an Object.
 *
 * Instances of object_ptr are best obtained through the ObjectManager::share
 * method of the responsible ObjectManager instance. Any constructor which is
 * not documented shall be considered an implementation detail and must not
 * be used explicitly.
 *
 * An object_ptr has three states:
 *
 * * *null*, in which case it is boolean false and was_valid() returns false.
 *   This is the case for instances created or assigned from nullptr.
 * * *dead*, in which case it is boolean false, get() returns nullptr and
 *   was_valid() returns true. This is the case if the object to which the
 *   object_ptr pointed has been deleted. The object_id() is still valid in
 *   that case.
 * * *alive*, in which case it is boolean true, get() returns the pointer to the
 *   object (and the ``*`` and ``->`` operators can be used normally) and
 *   was_valid() returns true. The object_id() is obviously valid.
 *
 * object_ptr instances support copying and moving, both by construction and
 * assignment, from other object_ptr instances as long as the types they
 * represent are also assignable. For downcasting, static_object_cast and
 * dynamic_object_cast are provided.
 *
 * Implicit construction and assignment from nullptr is also supported.
 *
 * The main use-case for object_ptr is for signals which cross over into
 * different threads. While the object_ptr itself is not thread-safe (as it
 * uses the underlying ObjectManager and Object, which are by itself not
 * thread-safe), it is useful if the signal emission and reception happens
 * synchronized.
 *
 * An example totally not related to this project is a simulation
 * loop which uses sig11::signal to emit signals, which get pushed
 * thread-safely into an event queue in a user-interface thread. The user
 * interface then acquires a lock which synchronises it with the object usage of
 * the simulation thread and only while it holds that lock it processes the
 * event queue.
 *
 * The user interface thread might receive multiple events related to a single
 * object, where the last event notes the deletion of the object. The object
 * has at that point already been deleted, and processing the previous events
 * without taking that into account would result in undefined behaviour. If the
 * events use object_ptr to pass the information about which object is
 * affected, the receiving thread can check whether the object is still alive
 * and act accordingly.
 *
 * @see ObjectManager
 */
template <typename T>
class object_ptr: private abstract_object_ptr
{
public:
    /**
     * Initialize a *null* object_ptr (see object_ptr class documentation).
     */
    object_ptr(std::nullptr_t = nullptr):
        abstract_object_ptr(nullptr),
        m_null(true),
        m_object_id(Object::NULL_OBJECT_ID)
    {

    }

    /**
     * Initialize an object_ptr by copying an existing object_ptr of the same
     * type.
     *
     * The new object_ptr will have the same state as the existing.
     */
    object_ptr(const object_ptr &src):
        abstract_object_ptr(src),
        m_null(src.m_null),
        m_object_id(src.m_object_id)
    {
        if (m_object) {
            add(this);
        }
    }

    /**
     * Initialize an object_ptr from an existing object. While it is currently
     * supported, the semantics for this may change in the future. It is
     * **highly discouraged** to use this function. Use ObjectManager::share
     * instead, with the correct object manager for the object.
     */
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    explicit object_ptr(U &object):
        abstract_object_ptr(object),
        m_null(false),
        m_object_id(object.object_id())
    {
        add(this);
    }

    /**
     * Initialize an object_ptr by copying an existing object_ptr of a
     * compatible type.
     *
     * A type is compatible if the pointer types ``U*`` and ``T*`` can be
     * assigned directly.
     *
     * The new object_ptr will have the same state as the original.
     */
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    object_ptr(const object_ptr<U> &src):
        abstract_object_ptr(src),
        m_null(src.m_null),
        m_object_id(src.m_object_id)
    {
        if (m_object) {
            add(this);
        }
    }

    /**
     * Initialize an object_ptr by moving an existing object_ptr of the same
     * type into the new instance.
     *
     * The new object_ptr will have the same state as the existing had at the
     * time of construction; the existing object_ptr will be a *null* pointer
     * afterwards.
     */
    object_ptr(object_ptr &&src):
        abstract_object_ptr(std::move(src)),
        m_null(src.m_null),
        m_object_id(src.m_object_id)
    {
        if (m_object) {
            remove(&src);
            add(this);
        }
        src.m_null = true;
    }

    /**
     * Initialize an object_ptr by moving an existing object_ptr of a
     * compatible type into the new instance.
     *
     * A type is compatible if the pointer types ``U*`` and ``T*`` can be
     * assigned directly.
     *
     * The new object_ptr will have the same state as the existing had at the
     * time of construction; the existing object_ptr will be a *null* pointer
     * afterwards.
     */
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    object_ptr(object_ptr<U> &&src):
        abstract_object_ptr(std::move(src)),
        m_null(src.m_null),
        m_object_id(src.m_object_id)
    {
        if (m_object) {
            remove(&src);
            add(this);
        }
        src.m_null = true;
    }

    /**
     * Copy-assign an existing object_ptr of the same type to this instance.
     */
    object_ptr &operator=(const object_ptr &src)
    {
        if (m_object) {
            remove(this);
        }

        abstract_object_ptr::operator=(src);
        m_object_id = src.m_object_id;
        m_null = src.m_null;

        if (m_object) {
            add(this);
        }

        return *this;
    }

    /**
     * Copy-assign an existing object_ptr of a compatible type to this
     * instance.
     *
     * A type is compatible if the pointer types ``U*`` and ``T*`` can be
     * assigned directly.
     */
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    object_ptr &operator=(const object_ptr<U> &src)
    {
        if (m_object) {
            remove(this);
        }

        abstract_object_ptr::operator=(src);
        m_object_id = src.m_object_id;
        m_null = src.m_null;

        if (m_object) {
            add(this);
        }

        return *this;
    }

    /**
     * Move-assign an existing object_ptr of the same type into this instance.
     */
    object_ptr &operator=(object_ptr &&src)
    {
        if (m_object) {
            remove(this);
        }
        if (src.m_object) {
            src.remove(&src);
        }
        abstract_object_ptr::operator=(std::move(src));
        m_object_id = src.m_object_id;
        m_null = src.m_null;
        src.m_null = true;

        if (m_object) {
            add(this);
        }

        return *this;
    }

    /**
     * Move-assign an existing object_ptr of a compatible type into this
     * instance.
     *
     * A type is compatible if the pointer types ``U*`` and ``T*`` can be
     * assigned directly.
     */
    template <typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value>::type>
    object_ptr &operator=(object_ptr<U> &&src)
    {
        if (m_object) {
            remove(this);
        }
        if (src.m_object) {
            src.remove(&src);
        }
        abstract_object_ptr::operator=(std::move(src));
        m_object_id = src.m_object_id;
        m_null = src.m_null;
        src.m_null = true;

        if (m_object) {
            add(this);
        }

        return *this;
    }

    /**
     * Set this pointer to null.
     */
    object_ptr &operator=(std::nullptr_t)
    {
        if (m_object) {
            remove(this);
        }
        abstract_object_ptr::operator=(nullptr);
        m_null = true;
        m_object_id = Object::NULL_OBJECT_ID;
        return *this;
    }

    ~object_ptr()
    {
        if (m_object) {
            remove(this);
        }
    }

private:
    bool m_null;
    Object::ID m_object_id;

    void add(abstract_object_ptr *ptr)
    {
        m_object->m_referees.emplace(ptr);
    }

    void remove(abstract_object_ptr *ptr)
    {
        auto iter = m_object->m_referees.find(ptr);
        m_object->m_referees.erase(iter);
    }

public:
    /**
     * Return the object this pointer points to.
     *
     * If the pointer is *null* or *dead*, this returns nullptr.
     */
    T *get() const
    {
        return static_cast<T*>(m_object);
    }

    /**
     * Release the object this pointer points to, setting the pointer to
     * *null*.
     *
     * This returns the object currently held by the pointer.
     */
    T *release()
    {
        T* result = get();
        if (result) {
            remove(this);
            m_null = true;
            m_object = nullptr;
        }
        return result;
    }

    T &operator*() const
    {
        return *get();
    }

    T *operator->() const
    {
        return get();
    }

    /**
     * Return true if the pointer is *alive*.
     */
    operator bool() const
    {
        return bool(m_object);
    }

    /**
     * Return true if the pointer is not *null*.
     *
     * This is false for all pointers which have been assigned nullptr.
     * Otherwise, this is true even after get() returns nullptr due to the
     * object having been destructed.
     */
    bool was_valid() const
    {
        return !m_null;
    }

    /**
     * Return the Object::ID of the object this pointer is referring to.
     *
     * If the pointer is *null*, the result is unspecified. If the pointer is
     * *dead*, the result is the original ID of the now-dead object.
     */
    inline Object::ID object_id() const
    {
        return m_object_id;
    }

    template <typename U> friend class object_ptr;
    friend class ObjectManager;
};


template <typename U, typename T>
static inline object_ptr<U> static_object_cast(object_ptr<T> &&src)
{
    if (!src) {
        return nullptr;
    }
    return object_ptr<U>(*static_cast<U*>(src.release()));
}

template <typename U, typename T>
static inline object_ptr<U> dynamic_object_cast(object_ptr<T> &&src)
{
    U *data = dynamic_cast<U*>(src.get());
    if (!data) {
        return nullptr;
    }
    return static_object_cast<U>(std::move(src));
}


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
         * Chunk matches the page size (which it doesn’t, on AMD64, currently).
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
    const Chunk *get_object_chunk(Object::ID object_id) const;
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
    const std::unique_ptr<Object> *get_object_ptr(Object::ID object_id) const;
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
     * Emplace the given object into the ObjectManager.
     *
     * If the object refers to an Object::ID which is already in use in this
     * ObjectManager, std::runtime_error is thrown.
     *
     * @param obj Object to emplace.
     * @throws std::runtime_error on ID conflict.
     */
    void emplace_object(std::unique_ptr<Object> &&obj);

    /**
     * Return a pointer to the Object associated with a given Object::ID.
     *
     * @param object_id ID to look up
     * @return Pointer to the Object associated with \a object_id. If no such
     * object exists, return nullptr.
     */
    Object *get_base(Object::ID object_id) const;

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
     * @name Management functions
     * These functions are used to allocate and release objects and
     * corresponding Object::ID numbers.
     */

    /**@{*/

    /**
     * Allocate a new object of type \a T and auto-assign an Object::ID.
     *
     * Object::ID assignment is implementation-defined; an ID may be
     * reassigned after the object to which it belongs has been deleted. IDs
     * are not used for multiple objects at the same time within the same
     * ObjectManager.
     *
     * The arguments are forwarded to the constructor of \a T, prepended with
     * the newly allocated Object::ID.
     *
     * The \a T instance is owned by the ObjectManager and will be deleted
     * automatically when ObjectManager is deleted. If you want to delete the
     * instance before that happens, use the kill().
     *
     * This method has strong exception safety against exceptions from the
     * constructor of \a T and those exceptions also propagate unchanged.
     *
     * If the ObjectManager runs out of IDs, std::runtime_error is thrown and
     * you are doomed. IDs are 64 bit integers. You do not simply run out of
     * 64 bit integers to identify objects. You simply cannot on current
     * architectures.
     *
     * @return Reference to the new \a T instance.
     * @throws std::runtime_error If the ObjectManager runs out of IDs.
     */
    template <typename T, typename... arg_ts>
    T &allocate(arg_ts&&... args)
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
     * Allocate a new object of type \a T with the given \a object_id.
     *
     * The basic functionality is identical to allocate(), but instead of
     * pulling the Object::ID from the internal pool of unused IDs, the given
     * ID is used.
     *
     * If the ID is already in use by a different object, std::runtime_error
     * is thrown.
     *
     * If the requested ID is equal to Object::NULL_OBJECT_ID, the call
     * behaves like allocate() and sources an unused ID from the allocation
     * pool.
     *
     * @param object_id Object::ID to use for the new object.
     * @return Reference to the new \a T instance.
     * @throws std::runtime_error on ID conflict
     */
    template <typename T, typename... arg_ts>
    T &emplace(const Object::ID object_id, arg_ts&&... args)
    {
        if (object_id == Object::NULL_OBJECT_ID) {
            return allocate<T>(std::forward<arg_ts>(args)...);
        }

        auto obj_ptr = std::make_unique<T>(object_id,
                                           std::forward<arg_ts>(args)...);
        T &obj = *obj_ptr;
        emplace_object(std::move(obj_ptr));
        return obj;
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
    T *get_safe(Object::ID object_id) const
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

    /**
     * Delete the given object managed by this ObjectManager.
     *
     * This call is equivalent to calling kill(Object::ID) with
     * ``object.object_id()``; the implications which follow from this if the
     * Object is owned by a different ObjectManager do apply (i.e. you will
     * either kill a different object or it is a no-op).
     *
     * @param object Object to kill.
     */
    inline void kill(Object &object)
    {
        kill(object.object_id());
    }

    /**@}*/

    /**
     * @name Sharing
     * These functions are used to share safe weak references to objects.
     */

    /**@{*/

    template <typename T>
    object_ptr<T> share(T &object) const
    {
        assert(get_safe<T>(object.object_id()) == &object);
        return object_ptr<T>(object);
    }

    template <typename T>
    object_ptr<T> share(Object::ID object_id) const
    {
        T *obj = get_safe<T>(object_id);
        if (!obj) {
            return nullptr;
        }
        return object_ptr<T>(*obj);
    }

    /**@}*/

public:
    /** @name Debugging
     * Methods used for debugging
     */
    /**@{*/

    /**
     * Dump the free list to the given output stream.
     *
     * @param out Output stream to write the free list to.
     */
    std::ostream &dump_free_list(std::ostream &out);

    /**@}*/
};


}


namespace std {

template <typename T>
struct hash<::sim::object_ptr<T> >
{
public:
    typedef ::sim::object_ptr<T> argument_type;
    typedef typename hash<void*>::result_type result_type;

private:
    hash<void*> m_ptr_hash;

public:
    result_type operator()(const ::sim::object_ptr<T> &ptr) const
    {
        return m_ptr_hash(ptr.get());
    }

};

}

#endif
