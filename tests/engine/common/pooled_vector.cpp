#include "ffengine/common/pooled_vector.hpp"

#include <catch.hpp>


using namespace ffe;

struct Movable
{
    Movable() = default;
    Movable(const Movable &ref) = delete;
    Movable &operator=(const Movable &ref) = delete;
    Movable(Movable &&src) = default;
    Movable &operator=(Movable &&src) = default;
};


struct Copyable
{
    Copyable() = default;
    Copyable(const Copyable &ref) = default;
    Copyable &operator=(const Copyable &ref) = default;
};


struct DefaultConstructible
{
    DefaultConstructible(const DefaultConstructible &ref) = delete;
    DefaultConstructible &operator=(const DefaultConstructible &ref) = delete;
    DefaultConstructible(DefaultConstructible &&src) = delete;
    DefaultConstructible &operator=(DefaultConstructible &&src) = delete;
};


TEST_CASE("common/utils/PooledVector/PooledVector()")
{
    PooledVector<int> v1;
    PooledVector<Movable> v2;
    PooledVector<Copyable> v3;
    PooledVector<DefaultConstructible> v4;
}

TEST_CASE("common/utils/PooledVector/PooledVector(std::size_t)")
{
    // NOTE: this currently eats all your RAM
    /*PooledVector<int> v1(10);
    CHECK_FALSE(v1.empty());

    PooledVector<Movable> v2;
    CHECK_FALSE(v2.empty());

    PooledVector<Copyable> v3;
    CHECK_FALSE(v3.empty());

    PooledVector<DefaultConstructible> v4;
    CHECK_FALSE(v4.empty());*/
}

TEST_CASE("common/utils/PooledVector/empty()")
{
    PooledVector<int> v;
    CHECK(v.empty());
}
