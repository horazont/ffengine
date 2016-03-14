#ifndef SCC_ENGINE_COMMON_STABLE_INDEX_VECTOR_H
#define SCC_ENGINE_COMMON_STABLE_INDEX_VECTOR_H

#include <array>
#include <algorithm>
#include <cassert>
#include <memory>
#include <iostream>
#include <vector>


namespace ffe {


class StableIndexVectorBase
{
protected:
    struct NoData {

    };

public:
    static constexpr std::size_t BLOCK_SIZE = 256;

};


/**
 * The StableIndexVector is a special container.
 *
 * To discuss the efficiency and performance of the vector, consider *N* the
 * number of elements which have been inserted into the vector and *H* the
 * number of holes in the vector. Holes are contiguous patches of memory
 * allocated for items by the vector which do not hold any item. Holes are
 * created or grow whenever items are erased from the vector and usually exist
 * at the edges of the vector.
 *
 * Using these definitions, the StableIndexVector has the following
 * characteristics:
 *
 * * Iterators are stable: Iterators only invalidate on shrink_to_fit(),
 *   clear() or move from/to the container, or if the item they are pointing at
 *   gets erased.
 * * Random access by index is O(1).
 * * Random access using iterator dereferenciation is O(1).
 * * Insertion is O(1), even if new memory needs to be allocated.
 * * Deletion is O(log H) in the average case. In the worst case, it is
 *   O(H log H) (which is when a new hole needs to be created and the deleted
 *   item is at the beginning of the container).
 * * Iteration steps are O(log H) in the worst case: whenever a hole is hit
 *   when stepping the iteration, a O(log H) search must be made for the next
 *   valid item. This includes a possible hole at the end of the container.
 * * Memory consumption is O(N), but always a multiple of the memory required
 *   for RandomAccessSetBase::BLOCK_SIZE objects of type \a T plus a few bytes
 *   overhead per item.
 * * The order in which inserted elements appear in the container is undefined.
 *   However, once inserted, elements never change order with respect to each
 *   other. New elements may appear between two existing elements though.
 * * The \a T items are only constructed when they are inserted into the
 *   container. When using emplace(), this happens in-place, without additional
 *   moving or copying.
 * * The \a T items are destructed immediately when they are erased from the
 *   container.
 */
template <typename T>
class StableIndexVector: public StableIndexVectorBase
{
public:
    using RawIndex = std::size_t;

private:
    struct Item
    {
        static struct inplace_t {} inplace;

        Item():
            m_valid(false)
        {

        }

        template <typename... arg_ts>
        explicit Item(inplace_t, arg_ts&&... args):
            m_valid(true)
        {
            new (&m_payload) T(std::forward<arg_ts>(args)...);
        }

        Item(const Item &src):
            m_valid(src.m_valid)
        {
            if (m_valid) {
                activate(src.m_payload);
            }
        }

        Item &operator=(Item &&src)
        {
            if (m_valid && src.m_valid) {
                m_payload = std::move(src.m_payload);
                src.deactivate();
            } else if (m_valid) {
                // !src.m_valid
                deactivate();
            } else if (src.m_valid) {
                // !m_valid
                activate(std::move(src.m_payload));
                src.deactivate();
            }
            return *this;
        }

        Item &operator=(const Item &src)
        {
            if (m_valid && src.m_valid) {
                m_payload = src.m_payload;
            } else if (m_valid) {
                // !src.m_valid
                deactivate();
            } else if (src.m_valid) {
                // !m_valid
                activate(src.m_payload);
            }
            return *this;
        }

        ~Item()
        {
            if (m_valid) {
                deactivate();
            }
        }

        bool m_valid;

        union {
            // replace with item index?
            NoData m_empty;
            T m_payload;
        };

        template <typename... arg_ts>
        void activate(arg_ts&&... args)
        {
            m_valid = true;
            new (&m_payload) T(std::forward<arg_ts>(args)...);
        }

        void deactivate()
        {
            m_payload.~T();
            m_valid = false;
        }
    };

    struct Block
    {
        std::array<Item, BLOCK_SIZE> m_items;
    };

    struct Region
    {
        Region(RawIndex first, RawIndex count, bool items_valid):
            m_items_valid(items_valid),
            m_first(first),
            m_count(count)
        {

        }

        bool m_items_valid;
        RawIndex m_first;
        RawIndex m_count;

        inline bool contains_index(const RawIndex index) const
        {
            return index >= m_first and index < m_first + m_count;
        }
    };

public:
    template <typename public_t, typename vec_t>
    class Iterator
    {
    public:
        using value_type = T;
        using reference = public_t&;
        using pointer = public_t*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

    protected:
        explicit Iterator(vec_t &vec, RawIndex pos):
            m_vec(&vec),
            m_pos(pos)
        {
            if (m_pos == m_vec->capacity()) {
                m_vec = nullptr;
                m_pos = 0;
            }
        }

    public:
        Iterator():
            m_vec(nullptr),
            m_pos(0)
        {

        }

        Iterator(const Iterator &src) = default;
        Iterator &operator=(const Iterator &src) = default;
        Iterator(Iterator &&src) = default;
        Iterator &operator=(Iterator &&src) = default;

        template <typename other_public_t, typename other_vec_t>
        Iterator(const Iterator<other_public_t, other_vec_t> &src):
            m_vec(src.m_vec),
            m_pos(src.m_pos)
        {

        }

    private:
        vec_t *m_vec;
        RawIndex m_pos;

    public:
        reference operator*() const
        {
            return (*m_vec)[m_pos];
        }

        pointer operator->() const
        {
            return &(*m_vec)[m_pos];
        }

        Iterator &operator++()
        {
            m_pos = m_vec->next(m_pos);
            if (m_pos == m_vec->capacity()) {
                m_vec = nullptr;
                m_pos = 0;
            }
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator result = *this;
            ++(*this);
            return result;
        }

        Iterator &operator--()
        {
            m_pos = m_vec->prev(m_pos);
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator result = *this;
            --(*this);
            return result;
        }

        bool operator==(const Iterator &other) const
        {
            return m_vec == other.m_vec and m_pos == other.m_pos;
        }

        bool operator!=(const Iterator &other) const
        {
            return !(*this == other);
        }

        inline RawIndex raw_index() const
        {
            return m_pos;
        }

    public:
        friend class StableIndexVector;
    };

    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator<T, StableIndexVector>;
    using const_iterator = Iterator<const T, const StableIndexVector>;

public:
    /**
     * Construct an empty vector.
     */
    StableIndexVector()
    {

    }

    /**
     * Construct a new vector with a copy of the contents from \a ref.
     *
     * @param ref A vector to copy contents from.
     *
     * @note This does not invoke shrink_to_fit(). This allows to re-use
     * RawIndex values from \a ref when working with the newly created vector.
     */
    StableIndexVector(const StableIndexVector &ref):
        m_blocks(),
        m_regions(ref.m_regions)
    {
        m_blocks.reserve(ref.m_blocks.size());
        for (const auto &src_block: ref.m_blocks) {
            m_blocks.emplace_back(new Block(*src_block));
        }
    }

    /**
     * Assign the contents of \a ref to this vector.
     *
     * @param ref A vector to copy contents from.
     *
     * @note This does not invoke shrink_to_fit(). This allows to re-use
     * RawIndex values from \a ref when working with the assigned-to vector.
     */
    StableIndexVector &operator=(const StableIndexVector &ref)
    {
        m_regions = ref.m_regions;
        m_blocks.resize(ref.m_blocks.size());
        for (std::size_t i = 0; i < ref.m_blocks.size(); ++i) {
            if (m_blocks[i]) {
                *m_blocks[i] = *ref.m_blocks[i];
            } else {
                m_blocks[i] = std::make_unique<Block>(*ref.m_blocks[i]);
            }
        }
        return *this;
    }

    /**
     * Construct a new vector and move the contents of \a src into it.
     *
     * @param src A vector to move contents from. After this operation, \a src
     * is in the same state as a newly created vector.
     *
     * @note This does not invoke shrink_to_fit(). This allows to re-use
     * RawIndex values from \a src when working with the newly created vector.
     */
    StableIndexVector(StableIndexVector &&src):
        m_blocks(std::move(src.m_blocks)),
        m_regions(std::move(src.m_regions))
    {
        src.m_blocks.clear();
        src.m_regions.clear();
    }

    /**
     * Move the contents from \a src into this vector.
     *
     * @param src A vector to move contents from. After this operation, \a src
     * is in the same state as a newly created vector.
     *
     * @note This does not invoke shrink_to_fit(). This allows to re-use
     * RawIndex values from \a src when working with the assigned-to vector.
     */
    StableIndexVector &operator=(StableIndexVector &&src)
    {
        m_blocks = std::move(src.m_blocks);
        m_regions = std::move(src.m_regions);
        src.m_blocks.clear();
        src.m_regions.clear();
        return *this;
    }

private:
    std::vector<std::unique_ptr<Block> > m_blocks;
    std::vector<Region> m_regions;

    typename std::vector<Region>::iterator find_empty_region()
    {
        auto iter = m_regions.begin();
        if (iter == m_regions.end()) {
            return iter;
        }

        if (iter->m_items_valid) {
            ++iter;
        }

        assert(iter == m_regions.end() || !iter->m_items_valid);
        return iter;
    }

    typename std::vector<Region>::iterator make_empty_region()
    {
        const std::size_t new_first = m_blocks.size()*BLOCK_SIZE;
        assert(m_regions.size() == 0 or m_regions.back().m_items_valid);

        m_blocks.emplace_back(new Block);
        m_regions.emplace_back(new_first, BLOCK_SIZE, false);
        return m_regions.end()-1;
    }

    /**
     * Take an arbitrary index from the region pointed to by \a region_iter and
     * return it, along with a new iterator pointing to the same region.
     *
     * If the region gets deleted, an iterator to the region which now holds
     * the returned index is returned. You can distinguish the cases by
     * inspecting the Region::m_items_valid member.
     *
     * @param region_iter
     * @return
     */
    std::tuple<RawIndex, typename std::vector<Region>::iterator> use_from_region(
            typename std::vector<Region>::iterator region_iter)
    {
        assert(!region_iter->m_items_valid);
        assert(region_iter->m_count > 0);

        // if this is not the first region, we can take the first index
        if (region_iter != m_regions.begin()) {
            RawIndex result = region_iter->m_first;
            auto prev = region_iter-1;
            assert(prev->m_items_valid);
            prev->m_count += 1;
            region_iter->m_first += 1;
            region_iter->m_count -= 1;
            if (region_iter->m_count == 0) {
                // return iterator pointing to the previous region
                auto next = m_regions.erase(region_iter);
                prev = next-1;
                if (next != m_regions.end()) {
                    assert(next->m_items_valid);
                    prev->m_count += next->m_count;
                    return std::make_tuple(result, m_regions.erase(next)-1);
                }
            }
            return std::make_tuple(result, region_iter);
        }

        // if this was the first region, we take the index from the end to
        // avoid inserting in the front of the regions vector
        RawIndex result = region_iter->m_first + region_iter->m_count - 1;
        auto next = region_iter + 1;
        if (next == m_regions.end()) {
            if (region_iter->m_count == 1) {
                // ha, wtf.
                region_iter->m_items_valid = true;
                return std::make_tuple(result, region_iter);
            }

            // no region where we could prepend, we have to insert a new one
            region_iter->m_count -= 1;
            m_regions.emplace_back(result, 1, true);
            return std::make_tuple(result, m_regions.begin());
        }

        // prepend to the next region
        assert(next->m_items_valid);
        next->m_first -= 1;
        next->m_count += 1;
        region_iter->m_count -= 1;
        if (region_iter->m_count == 0) {
            return std::make_tuple(result, m_regions.erase(region_iter));
        } else {
            return std::make_tuple(result, region_iter);
        }
    }

    Item &item_by_index(RawIndex index)
    {
        std::size_t block_index = index / BLOCK_SIZE;
        std::size_t item_index = index % BLOCK_SIZE;
        assert(block_index < m_blocks.size());
        return m_blocks[block_index]->m_items[item_index];
    }

    const Item &item_by_index(RawIndex index) const
    {
        std::size_t block_index = index / BLOCK_SIZE;
        std::size_t item_index = index % BLOCK_SIZE;
        assert(block_index < m_blocks.size());
        return m_blocks[block_index]->m_items[item_index];
    }

    typename std::vector<Region>::iterator region_by_index(RawIndex index)
    {
        if (index >= capacity()) {
            // simple :-)
            return m_regions.end();
        }

        struct Foo
        {
            inline bool operator()(const Region &region, RawIndex index) const
            {
                return region.m_first < index;
            }

            inline bool operator()(RawIndex index, const Region &region) const
            {
                return index < region.m_first;
            }
        };

        auto iter = std::upper_bound(m_regions.begin(),
                                     m_regions.end(),
                                     index,
                                     Foo());
        if (iter == m_regions.begin()) {
            return m_regions.end();
        }
        if (iter == m_regions.end()) {
            // it might be the last one, if it isn’t, we alias to end()
            --iter;
            if (iter->m_first + iter->m_count < index) {
                return m_regions.end();
            }
        } else {
            --iter;
        }

        assert(iter->contains_index(index));
        return iter;
    }

    typename std::vector<Region>::const_iterator region_by_index(RawIndex index) const
    {
        if (index >= capacity()) {
            // simple :-)
            return m_regions.end();
        }

        struct Foo
        {
            inline bool operator()(const Region &region, RawIndex index) const
            {
                return region.m_first < index;
            }

            inline bool operator()(RawIndex index, const Region &region) const
            {
                return index < region.m_first;
            }
        };

        auto iter = std::upper_bound(m_regions.begin(),
                                     m_regions.end(),
                                     index,
                                     Foo());
        if (iter == m_regions.begin()) {
            return m_regions.end();
        }
        if (iter == m_regions.end()) {
            // it might be the last one, if it isn’t, we alias to end()
            --iter;
            if (iter->m_first + iter->m_count < index) {
                return m_regions.end();
            }
        } else {
            --iter;
        }

        assert(iter->contains_index(index));
        return iter;
    }

    inline std::tuple<RawIndex, typename std::vector<Region>::const_iterator> map_and_normalise(const RawIndex curr) const
    {
        auto iter = region_by_index(curr);
        if (iter == m_regions.end()) {
            // behind end
            return std::make_tuple(capacity(), iter);
        }
        return std::make_tuple(curr, iter);
    }

    inline RawIndex next(const RawIndex curr) const
    {
        if (curr+1 < capacity()) {
            const Item &next_item = item_by_index(curr+1);
            if (next_item.m_valid) {
                // this is simple :-)
                return curr+1;
            }
        }

        typename std::vector<Region>::const_iterator iter;
        RawIndex mapped;
        std::tie(mapped, iter) = map_and_normalise(curr);
        if (iter == m_regions.end()) {
            // mapped points to behind-the-end
            return mapped;
        }

        if (!iter->m_items_valid) {
            // if it points to an invalid item, we still move to the next valid item
            ++iter;
            if (iter == m_regions.end()) {
                // this was the last one.
                return capacity();
            }
            assert(iter->m_items_valid);
            return iter->m_first;
        } else {
            RawIndex result = curr + 1;
            if (iter->contains_index(result)) {
                return result;
            }
            ++iter;
            if (iter == m_regions.end()) {
                // behind the end
                return capacity();
            }
            assert(!iter->m_items_valid);
            ++iter;
            if (iter == m_regions.end()) {
                // behind the end
                return capacity();
            }
            assert(iter->m_items_valid);
            return iter->m_first;
        }
    }

    RawIndex first_index() const
    {
        if (empty()) {
            return capacity();
        }
        if (m_regions[0].m_items_valid) {
            return m_regions[0].m_first;
        } else {
            return m_regions[1].m_first;
        }
    }

public:
    /**
     * Return the current capacity of the vector.
     *
     * This is always a multiple of StableIndexVectorBase::BLOCK_SIZE.
     */
    inline std::size_t capacity() const
    {
        return m_blocks.size() * BLOCK_SIZE;
    }

    /**
     * Return the current size of the vector.
     *
     * The size is the number of items which have been inserted into the
     * vector. Note that this operation is O(H).
     */
    inline std::size_t size() const
    {
        std::size_t count = 0;
        for (const Region &region: m_regions) {
            if (region.m_items_valid) {
                count += region.m_count;
            }
        }
        return count;
    }

    /**
     * Return true if the container is empty, false otherwise.
     *
     * In contrast to size(), this is O(1).
     */
    inline bool empty() const
    {
        return m_regions.size() == 0 or (m_regions.size() == 1 and !m_regions[0].m_items_valid);
    }

    /**
     * Return the number of blocks which are allocated. Each block may hold
     * up to StableIndexVectorBase::BLOCK_SIZE items of \a T.
     */
    inline std::size_t blocks() const
    {
        return m_blocks.size();
    }

    /**
     * Return the number of contiguous regions of used and unused item slots.
     *
     * A large number indicates a lot of fragmentation and leads to slow
     * iteration. Random deletions cause this number to rise (see the class
     * documentation of StableIndexVector for more info), while shrink_to_fit()
     * lowers it deterministically to 2 or less.
     */
    inline std::size_t regions() const
    {
        return m_regions.size();
    }

    /**
     * Create a new \a T in-place. The \a args are forwarded to the constructor
     * of \a T.
     *
     * @return The iterator pointing at the newly created element.
     */
    template <typename... arg_ts>
    inline iterator emplace(arg_ts&&... args)
    {
        RawIndex new_index;
        {
            auto iter = find_empty_region();
            if (iter == m_regions.end()) {
                iter = make_empty_region();
            }
            std::tie(new_index, iter) = use_from_region(iter);
        }

        Item &item = item_by_index(new_index);
        assert(!item.m_valid);
        item.activate(std::forward<arg_ts>(args)...);

        return iterator(*this, new_index);
    }

    /**
     * Access the element at the given raw \a index.
     *
     * The usual semantics of such an access apply: if there is no such element
     * (e.g. due to an out-of-bounds index or because the index points at a
     * hole), the behaviour is undefined :-).
     */
    inline T &operator[](RawIndex index)
    {
        Item &item = item_by_index(index);
        assert(item.m_valid);
        return item.m_payload;
    }

    /**
     * @copydoc operator[](RawIndex)
     */
    inline const T &operator[](RawIndex index) const
    {
        const Item &item = item_by_index(index);
        assert(item.m_valid);
        return item.m_payload;
    }

    /**
     * Erase the element at the given iterator location.
     *
     * If the element is not adjacent to a hole, a new hole is created (slowing
     * down iteration from the previous to the next element). Otherwise, the
     * existing hole grows (without additional performance impact).
     *
     * @param index The iterator pointing at the element to erase.
     * @return An iterator pointing to the next element behind the element
     * which was erased.
     */
    inline iterator erase(const_iterator index)
    {
        const RawIndex raw_index = index.m_pos;
        Item &item = item_by_index(raw_index);
        assert(item.m_valid);
        item.deactivate();

        RawIndex result = next(raw_index);
        auto region_iter = region_by_index(raw_index);
        assert(region_iter != m_regions.end());
        assert(region_iter->m_items_valid);

        if (region_iter->m_count == 1) {
            // simply re-purpose the region
            if (region_iter != m_regions.begin()) {
                auto prev = region_iter - 1;
                assert(!prev->m_items_valid);
                prev->m_count += 1;
                auto next = m_regions.erase(region_iter);
                prev = next - 1;
                if (next != m_regions.end()) {
                    assert(!next->m_items_valid);
                    prev->m_count += next->m_count;
                    m_regions.erase(next);
                }
            } else if (region_iter != m_regions.end()) {
                auto next = region_iter + 1;
                assert(!next->m_items_valid);
                next->m_first -= 1;
                next->m_count += 1;
                m_regions.erase(region_iter);
            } else {
                region_iter->m_items_valid = false;
            }
        } else if (region_iter->m_first == raw_index) {
            if (region_iter == m_regions.begin()) {
                // we must insert a new region at the beginning
                auto new_region = m_regions.emplace(region_iter, raw_index, 1, false);
                region_iter = new_region + 1;
                region_iter->m_first += 1;
                region_iter->m_count -= 1;
            } else {
                auto prev = region_iter - 1;
                assert(!prev->m_items_valid);
                prev->m_count += 1;
                region_iter->m_first += 1;
                region_iter->m_count -= 1;
            }
        } else if (region_iter->m_first + region_iter->m_count - 1 == raw_index) {
            if (region_iter == m_regions.end()-1) {
                // we must insert a new region at the end
                m_regions.emplace_back(raw_index, 1, false);
                region_iter = m_regions.end()-2;
                region_iter->m_count -= 1;
            } else {
                // prepend to next region
                auto next = region_iter + 1;
                next->m_count += 1;
                next->m_first -= 1;
                region_iter->m_count -= 1;
            }
        } else {
            // general case, index is in the middle of the region
            auto new_used_region = m_regions.emplace(region_iter + 1, raw_index+1, 0, true);
            auto new_freed_region = m_regions.emplace(new_used_region, raw_index, 1, false);
            new_used_region = new_freed_region + 1;
            region_iter = new_freed_region - 1;
            new_used_region->m_count = region_iter->m_count - (raw_index - region_iter->m_first) - 1;
            region_iter->m_count = raw_index - region_iter->m_first;
        }

        return iterator(*this, result);
    }

    /**
     * Return an iterator pointing to the first item.
     *
     * If the container is empty, this is equal to end().
     */
    iterator begin()
    {
        return iterator(*this, first_index());
    }

    /**
     * Return a const iterator pointing to the first item.
     *
     * If the container is empty, this is equal to end().
     */
    const_iterator begin() const
    {
        return const_iterator(*this, first_index());
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    /**
     * Return a past-the-end iterator.
     */
    iterator end()
    {
        return iterator();
    }

    /**
     * Return a past-the-end const iterator.
     */
    const_iterator end() const
    {
        return const_iterator();
    }

    const_iterator cend() const
    {
        return end();
    }

    /**
     * Insert the given \a value into the container. This copy-constructs a new
     * \a T in the container from \a value.
     */
    iterator insert(const T &value)
    {
        return emplace(value);
    }

    /**
     * Insert the given \a value into the container. This move-constructs a new
     * \a T in the container from \a value.
     */
    iterator insert(T &&value)
    {
        return emplace(std::move(value));
    }

    /**
     * Clear the vector.
     *
     * This invalidates all iterators and is O(N) (all the destructors need to
     * be called). This does explicitly not release the memory reserved for
     * items; call shrink_to_fit() after clear() to do that.
     */
    void clear()
    {
        for (const Region &region: m_regions) {
            if (!region.m_items_valid) {
                continue;
            }
            for (RawIndex i = region.m_first;
                 i < region.m_count + region.m_first;
                 ++i)
            {
                Item &item = item_by_index(i);
                assert(item.m_valid);
                item.deactivate();
            }
        }

        m_regions.clear();
        m_regions.emplace_back(0, capacity(), false);
    }

    /**
     * Convert the given RawIndex to an \link iterator. If the \a index is
     * out of bounds or points to an invalid location, end() is returned.
     *
     * @param index The raw index to use.
     * @return New iterator pointing to the element at the given raw index, or
     * end().
     */
    iterator iterator_from_index(RawIndex index)
    {
        typename std::vector<Region>::const_iterator region_iter;
        std::tie(index, region_iter) = map_and_normalise(index);
        if (region_iter == m_regions.end() or !region_iter->m_items_valid) {
            return end();
        }
        if (!item_by_index(index).m_valid) {
            return end();
        }
        return iterator(*this, index);
    }

    /**
     * @copydoc iterator_from_index(RawIndex)
     */
    const_iterator iterator_from_index(RawIndex index) const
    {
        typename std::vector<Region>::const_iterator region_iter;
        std::tie(index, region_iter) = map_and_normalise(index);
        if (region_iter == m_regions.end() or !region_iter->m_items_valid) {
            return end();
        }
        if (!item_by_index(index).m_valid) {
            return end();
        }
        return const_iterator(*this, index);
    }

    /**
     * Defragment the container, moving all live items to the beginning and
     * leaving a huge hole at the end of the container.
     *
     * This invalidates all iterators.
     *
     * For this to work, \a T must be move-assignable and move-constructable.
     *
     * This operation is O(N) in the worst case (if there is a hole at the
     * beginning).
     *
     * @note This does not release any memory and thus leaves the capacity()
     * unchanged. To release the memory, either call shrink_utils_to_fit()
     * afterwards or use shrink_to_fit() directly.
     */
    void defrag()
    {
        if (empty()) {
            m_blocks.clear();
            m_regions.clear();
        }

        const std::size_t size = this->size();

        // we use the old regions to step through the blocks :-)
        RawIndex dest = 0;
        for (const Region &region: m_regions) {
            if (!region.m_items_valid) {
                continue;
            }
            for (RawIndex src = region.m_first;
                 src < region.m_first + region.m_count;
                 ++src, ++dest)
            {
                if (src == dest) {
                    continue;
                }
                assert(src > dest);
                Item &src_item = item_by_index(src);
                Item &dest_item = item_by_index(dest);
                assert(src_item.m_valid);
                assert(!dest_item.m_valid);
                dest_item = std::move(src_item);
            }
        }

        m_regions.clear();
        m_regions.emplace_back(0, size, true);
        if (size < capacity()) {
            m_regions.emplace_back(size, capacity(), false);
        }
    }

    /**
     * Call defrag() and trim().
     *
     * This reduces the memory footprint of the container to the minimum
     * possible. As it calls defrag(), all iterators are invalidated afterwards.
     */
    void shrink_to_fit()
    {
        defrag();
        trim();
    }

    /**
     * Trim off unused capacity() from the end of the container and from
     * internal bookkeeping.
     *
     * This does not make any attempt to move items to free more space at the
     * end of the container (use defrag() or shrink_to_fit() for that). No
     * iterators are invalidated.
     *
     * This is O(H) in the worst case (if internal bookkeeping needs to move
     * due to reallocation to a smaller memory region).
     */
    void trim()
    {
        if (m_regions.size() > 0 and !m_regions.back().m_items_valid) {
            // the last block(s) may be empty :-)
            Region &last = m_regions.back();
            // only fully covered blocks are unneeded
            const std::size_t unneeded_blocks = last.m_count / BLOCK_SIZE;
            m_blocks.resize(m_blocks.size() - unneeded_blocks);
            last.m_count -= unneeded_blocks*BLOCK_SIZE;
            if (last.m_count == 0) {
                m_regions.erase(m_regions.end()-1);
            }
        }
        m_regions.shrink_to_fit();
        m_blocks.shrink_to_fit();
    }

};

}


#endif
