#ifndef SCC_ENGINE_COMMON_POOLED_VECTOR_H
#define SCC_ENGINE_COMMON_POOLED_VECTOR_H

#include <algorithm>
#include <vector>

namespace ffe {


template <typename T, std::size_t _pool_size = 0>
class PooledVector
{
private:
    // this is not for aligning to pages, but to get a reasonable memory usage
    // vs. efficiency tradeoff
    static constexpr std::size_t page_pool_size = 4096 / sizeof(T);
    static constexpr std::size_t hugepage_pool_size = (1024*2048) / sizeof(T);

public:
    static constexpr std::size_t pool_size = (_pool_size > 0 ? _pool_size : (page_pool_size >= 128 ? page_pool_size : (hugepage_pool_size >= 128 ? hugepage_pool_size : 128)));
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

private:
    struct Block
    {
        Block():
            storage()
        {
            storage.reserve(pool_size);
        }

        explicit Block(std::size_t n):
            storage()
        {
            if (n < pool_size) {
                storage.reserve(pool_size);
            }
            storage.resize(n);
        }

        Block(const Block &ref) = default;
        Block &operator=(const Block &ref) = default;
        Block(Block &&src) = default;
        Block &operator=(Block &&src) = default;

        std::vector<T> storage;
    };

public:
    class Iterator
    {

    };

public:
    PooledVector() = default;

    explicit PooledVector(std::size_t n)
    {
        while (n > 0) {
            m_blocks.emplace_back(n);
        }
    }

    PooledVector(std::initializer_list<T> items):
        PooledVector(items.begin(), items.end())
    {

    }

    template <typename InputIt>
    PooledVector(InputIt first, InputIt last)
    {

    }

    PooledVector(const PooledVector &src) = default;
    PooledVector &operator=(const PooledVector &src) = default;

private:
    std::vector<Block> m_blocks;

public:
    inline bool empty() const
    {
        return std::all_of(
                    m_blocks.begin(), m_blocks.end(),
                    [](const Block &block){return block.storage.empty();});
    }

    inline std::size_t size() const
    {
        std::size_t sum = 0;
        for (const Block &block: m_blocks) {
            sum += block.storage.size();
        }
        return sum;
    }

};


}

#endif
