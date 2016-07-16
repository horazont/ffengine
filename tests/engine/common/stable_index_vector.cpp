#include "ffengine/common/stable_index_vector.hpp"

#include <unordered_set>

#include <catch.hpp>

using namespace ffe;


struct Event
{

    enum Type {
        DEFAULT_CONSTRUCT,
        ARGUMENT_CONSTRUCT,
        COPY_CONSTRUCT,
        MOVE_CONSTRUCT,
        COPY_ASSIGN,
        MOVE_ASSIGN,
        DESTRUCT,
    };

    Event() = delete;

    Event(Type t):
        type(t)
    {

    }

    Type type;

    bool operator==(const Event &other) const
    {
        return type == other.type;
    }

    bool operator!=(const Event &other) const
    {
        return type != other.type;
    }
};

std::ostream &operator<<(std::ostream &stream, const Event &ev)
{
    stream << "Event(";
    switch (ev.type) {
    case Event::DEFAULT_CONSTRUCT:
    {
        stream << "DEFAULT_CONSTRUCT";
        break;
    }
    case Event::ARGUMENT_CONSTRUCT:
    {
        stream << "ARGUMENT_CONSTRUCT";
        break;
    }
    case Event::COPY_CONSTRUCT:
    {
        stream << "COPY_CONSTRUCT";
        break;
    }
    case Event::MOVE_CONSTRUCT:
    {
        stream << "MOVE_CONSTRUCT";
        break;
    }
    case Event::COPY_ASSIGN:
    {
        stream << "COPY_ASSIGN";
        break;
    }
    case Event::MOVE_ASSIGN:
    {
        stream << "MOVE_ASSIGN";
        break;
    }
    case Event::DESTRUCT:
    {
        stream << "DESTRUCT";
        break;
    }
    }

    return stream << ")";
}


class NonTriviallyConstructable
{
public:
    using Event = ::Event;
    static std::vector<Event> recorded_events;

public:
    NonTriviallyConstructable():
        n(0)
    {
        recorded_events.emplace_back(Event::DEFAULT_CONSTRUCT);
    }

    NonTriviallyConstructable(int n):
        n(n)
    {
        recorded_events.emplace_back(Event::ARGUMENT_CONSTRUCT);
    }

    NonTriviallyConstructable(const NonTriviallyConstructable &ref):
        n(ref.n)
    {
        recorded_events.emplace_back(Event::COPY_CONSTRUCT);
    }

    NonTriviallyConstructable(NonTriviallyConstructable &&src):
        n(src.n)
    {
        recorded_events.emplace_back(Event::MOVE_CONSTRUCT);
        src.n = 0xdeadbeef;
    }

    NonTriviallyConstructable &operator=(const NonTriviallyConstructable &ref)
    {
        n = ref.n;
        recorded_events.emplace_back(Event::COPY_ASSIGN);
        return *this;
    }

    NonTriviallyConstructable &operator=(NonTriviallyConstructable &&src)
    {
        n = src.n;
        recorded_events.emplace_back(Event::MOVE_ASSIGN);
        src.n = 0xdeadbeef;
        return *this;
    }

    ~NonTriviallyConstructable()
    {
        recorded_events.emplace_back(Event::DESTRUCT);
    }

    int n;

    inline bool operator==(const NonTriviallyConstructable &other) const
    {
        return n == other.n;
    }

    inline bool operator!=(const NonTriviallyConstructable &other) const
    {
        return n != other.n;
    }

};

namespace std {

template<>
struct hash<NonTriviallyConstructable>
{
private:
    using SubType = std::hash<int>;

public:
    using argument_type = NonTriviallyConstructable;
    using result_type = typename SubType::result_type;

private:
    std::hash<int> m_hash;

public:
    inline result_type operator()(const argument_type &value) const
    {
        return m_hash(value.n);
    }

};

}

std::vector<NonTriviallyConstructable::Event> NonTriviallyConstructable::recorded_events;


using TestVector = ffe::StableIndexVector<NonTriviallyConstructable>;


TEST_CASE("common/StableIndexVector/StableIndexVector()")
{
    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv;
    CHECK(tv.empty());
    CHECK(tv.size() == 0);
    CHECK(tv.capacity() == 0);

    std::vector<NonTriviallyConstructable::Event> ref;
    CHECK(NonTriviallyConstructable::recorded_events == ref);
}

TEST_CASE("common/StableIndexVector/emplace_one")
{
    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv;
    tv.emplace();
    CHECK(!tv.empty());
    CHECK(tv.size() == 1);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::DEFAULT_CONSTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);
}

TEST_CASE("common/StableIndexVector/emplace_several")
{
    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv;
    tv.emplace(1);
    tv.emplace(2);
    tv.emplace(3);
    tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);
}

TEST_CASE("common/StableIndexVector/emplace_deref")
{
    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv;
    auto
            index1 = tv.emplace(1),
            index2 = tv.emplace(2),
            index3 = tv.emplace(3),
            index4 = tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    CHECK(index1->n == 1);
    CHECK(index2->n == 2);
    CHECK(index3->n == 3);
    CHECK(index4->n == 4);
}

TEST_CASE("common/StableIndexVector/erase_from_center_and_reemplace")
{
    TestVector tv;
    auto
            index1 = tv.emplace(1),
            index2 = tv.emplace(2),
            index3 = tv.emplace(3),
            index4 = tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    for (std::size_t i = 4; i < TestVector::BLOCK_SIZE; ++i) {
        tv.emplace(i+1);
    }
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    NonTriviallyConstructable::recorded_events.clear();

    tv.erase(index2);
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity()-1);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 3);

    auto index5 = tv.emplace(5);
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    CHECK(index5 == index2);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    CHECK(index1->n == 1);
    CHECK(index5->n == 5);
    CHECK(index3->n == 3);
    CHECK(index4->n == 4);
}

TEST_CASE("common/StableIndexVector/erase_from_head_and_reemplace")
{
    TestVector tv;
    auto
            index1 = tv.emplace(1),
            index2 = tv.emplace(2),
            index3 = tv.emplace(3),
            index4 = tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    for (std::size_t i = 4; i < TestVector::BLOCK_SIZE; ++i) {
        tv.emplace(i+1);
    }
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    NonTriviallyConstructable::recorded_events.clear();

    tv.erase(index1);
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity()-1);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    auto index5 = tv.emplace(5);
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    CHECK(index5 == index1);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    CHECK(index5->n == 5);
    CHECK(index2->n == 2);
    CHECK(index3->n == 3);
    CHECK(index4->n == 4);
}

TEST_CASE("common/StableIndexVector/erase_from_tail_and_reemplace")
{
    TestVector tv;
    auto
            index1 = tv.emplace(1),
            index2 = tv.emplace(2),
            index3 = tv.emplace(3),
            index4 = tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    TestVector::iterator last;
    for (std::size_t i = 4; i < TestVector::BLOCK_SIZE; ++i) {
        last = tv.emplace(i+1);
    }
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    NonTriviallyConstructable::recorded_events.clear();

    tv.erase(last);
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity()-1);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    auto index5 = tv.emplace(5);
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    CHECK(index5 == last);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    CHECK(index1->n == 1);
    CHECK(index2->n == 2);
    CHECK(index3->n == 3);
    CHECK(index4->n == 4);
    CHECK(index5->n == 5);
}

TEST_CASE("common/StableIndexVector/erase_multiple_from_tail_and_reemplace")
{
    TestVector tv;
    auto
            index1 = tv.emplace(1),
            index2 = tv.emplace(2),
            index3 = tv.emplace(3),
            index4 = tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    std::array<TestVector::iterator, 4> last_few;
    for (std::size_t i = 4; i < TestVector::BLOCK_SIZE; ++i) {
        last_few[i%4] = tv.emplace(i+1);
    }
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    NonTriviallyConstructable::recorded_events.clear();

    for (auto iter: last_few) {
        tv.erase(iter);
    }
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity()-4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    int i = 10;
    for (auto &iter: last_few) {
        iter = tv.emplace(i++);
    }
    CHECK(!tv.empty());
    CHECK(tv.size() == tv.capacity());
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    CHECK(index1->n == 1);
    CHECK(index2->n == 2);
    CHECK(index3->n == 3);
    CHECK(index4->n == 4);
}

TEST_CASE("common/StableIndexVector/emplace_several_and_iterate")
{
    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv;
    tv.emplace(1);
    tv.emplace(2);
    tv.emplace(3);
    tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    std::unordered_set<NonTriviallyConstructable> found(tv.begin(), tv.end());
    std::unordered_set<NonTriviallyConstructable> ref({
                                                          {1},
                                                          {2},
                                                          {3},
                                                          {4}
                                                      });
    CHECK(found == ref);
}

TEST_CASE("common/StableIndexVector/clear")
{
    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv;
    tv.emplace(1);
    tv.emplace(2);
    tv.emplace(3);
    tv.emplace(4);
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);

    tv.clear();
    CHECK(tv.empty());
    CHECK(tv.size() == 0);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 1);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::ARGUMENT_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                          {NonTriviallyConstructable::Event::DESTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);
}

TEST_CASE("common/StableIndexVector/StableIndexVector(const StableIndexVector&)")
{
    TestVector tv;

    std::array<TestVector::iterator, 4> indices({
                                                    tv.emplace(1),
                                                    tv.emplace(2),
                                                    tv.emplace(3),
                                                    tv.emplace(4),
                                                });
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);
    tv.erase(indices[2]);

    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv2(tv);
    CHECK(!tv.empty());
    CHECK(tv.size() == 3);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 4);
    CHECK(!tv2.empty());
    CHECK(tv2.size() == 3);
    CHECK(tv2.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv2.blocks() == 1);
    CHECK(tv2.regions() == 4);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::COPY_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::COPY_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::COPY_CONSTRUCT}
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    std::vector<NonTriviallyConstructable> from1(tv.begin(), tv.end());
    std::vector<NonTriviallyConstructable> from2(tv2.begin(), tv2.end());
    CHECK(from1 == from2);

    for (const auto &index: indices) {
        if (index == indices[2]) {
            continue;
        }
        TestVector::RawIndex raw = index.raw_index();
        auto iter2 = tv2.iterator_from_index(raw);
        CHECK(iter2 != tv2.end());
        CHECK(*iter2 == *index);
    }
}

TEST_CASE("common/StableIndexVector/StableIndexVector &operator=(const StableIndexVector&)")
{
    TestVector tv;

    std::array<TestVector::iterator, 4> indices({
                                                    tv.emplace(1),
                                                    tv.emplace(2),
                                                    tv.emplace(3),
                                                    tv.emplace(4),
                                                });
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);
    tv.erase(indices[2]);

    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv2;
    tv2 = tv;
    CHECK(!tv.empty());
    CHECK(tv.size() == 3);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 4);
    CHECK(!tv2.empty());
    CHECK(tv2.size() == 3);
    CHECK(tv2.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv2.blocks() == 1);
    CHECK(tv2.regions() == 4);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {NonTriviallyConstructable::Event::COPY_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::COPY_CONSTRUCT},
                                                          {NonTriviallyConstructable::Event::COPY_CONSTRUCT}
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    std::vector<NonTriviallyConstructable> from1(tv.begin(), tv.end());
    std::vector<NonTriviallyConstructable> from2(tv2.begin(), tv2.end());
    CHECK(from1 == from2);

    for (const auto &index: indices) {
        if (index == indices[2]) {
            continue;
        }
        TestVector::RawIndex raw = index.raw_index();
        auto iter2 = tv2.iterator_from_index(raw);
        CHECK(iter2 != tv2.end());
        CHECK(*iter2 == *index);
    }
}

TEST_CASE("common/StableIndexVector/StableIndexVector(StableIndexVector&&)")
{
    TestVector tv;

    std::array<TestVector::iterator, 4> indices({
                                                    tv.emplace(1),
                                                    tv.emplace(2),
                                                    tv.emplace(3),
                                                    tv.emplace(4),
                                                });
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);
    tv.erase(indices[2]);
    std::vector<NonTriviallyConstructable> from1(tv.begin(), tv.end());

    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv2(std::move(tv));
    CHECK(tv.empty());
    CHECK(tv.size() == 0);
    CHECK(tv.capacity() == 0);
    CHECK(tv.blocks() == 0);
    CHECK(tv.regions() == 0);
    CHECK(!tv2.empty());
    CHECK(tv2.size() == 3);
    CHECK(tv2.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv2.blocks() == 1);
    CHECK(tv2.regions() == 4);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    std::vector<NonTriviallyConstructable> from2(tv2.begin(), tv2.end());
    CHECK(from1 == from2);
    CHECK(indices[0]->n == 1);
    CHECK(indices[1]->n == 2);
    CHECK(indices[3]->n == 4);
}

TEST_CASE("common/StableIndexVector/StableIndexVector &operator=(StableIndexVector&&)")
{
    TestVector tv;

    std::array<TestVector::iterator, 4> indices({
                                                    tv.emplace(1),
                                                    tv.emplace(2),
                                                    tv.emplace(3),
                                                    tv.emplace(4),
                                                });
    CHECK(!tv.empty());
    CHECK(tv.size() == 4);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 2);
    tv.erase(indices[2]);
    std::vector<NonTriviallyConstructable> from1(tv.begin(), tv.end());

    NonTriviallyConstructable::recorded_events.clear();

    TestVector tv2;
    tv2 = std::move(tv);
    CHECK(tv.empty());
    CHECK(tv.size() == 0);
    CHECK(tv.capacity() == 0);
    CHECK(tv.blocks() == 0);
    CHECK(tv.regions() == 0);
    CHECK(!tv2.empty());
    CHECK(tv2.size() == 3);
    CHECK(tv2.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv2.blocks() == 1);
    CHECK(tv2.regions() == 4);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    std::vector<NonTriviallyConstructable> from2(tv2.begin(), tv2.end());
    CHECK(from1 == from2);
    CHECK(indices[0]->n == 1);
    CHECK(indices[1]->n == 2);
    CHECK(indices[3]->n == 4);
}

TEST_CASE("common/StableIndexVector/defrag")
{
    TestVector tv;
    TestVector::IndexMap map;

    std::array<TestVector::iterator, 4> indices({
                                                    tv.emplace(1),
                                                    tv.emplace(2),
                                                    tv.emplace(3),
                                                    tv.emplace(4),
                                                });

    for (std::size_t i = 4; i < TestVector::BLOCK_SIZE+1; ++i) {
        tv.emplace(i+1);
    }

    CHECK(!tv.empty());
    CHECK(tv.size() == TestVector::BLOCK_SIZE+1);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE*2);
    CHECK(tv.blocks() == 2);
    CHECK(tv.regions() == 2);
    tv.erase(indices[1]);
    tv.erase(indices[3]);

    std::vector<NonTriviallyConstructable> from1(tv.begin(), tv.end());

    NonTriviallyConstructable::recorded_events.clear();

    tv.defrag(&map);
    CHECK(!tv.empty());
    CHECK(tv.size() == TestVector::BLOCK_SIZE-1);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE*2);
    CHECK(tv.blocks() == 2);
    CHECK(tv.regions() == 2);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                          {Event::MOVE_CONSTRUCT},
                                                          {Event::DESTRUCT},
                                                          {Event::MOVE_CONSTRUCT},
                                                          {Event::DESTRUCT},
                                                          {Event::MOVE_CONSTRUCT},
                                                          {Event::DESTRUCT},
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    std::vector<NonTriviallyConstructable> from2(tv.begin(), tv.end());
    CHECK(from1 == from2);

    CHECK(tv.iterator_from_index(map.map(indices[0].raw_index()))->n == 1);
    CHECK(tv.iterator_from_index(map.map(indices[2].raw_index()))->n == 3);

    CHECK(map.map(indices[1].raw_index()) == IndexMapBase::INVALID_INDEX);
    CHECK(map.map(indices[3].raw_index()) == IndexMapBase::INVALID_INDEX);
}

TEST_CASE("common/StableIndexVector/trim")
{
    TestVector tv;

    std::array<TestVector::iterator, 4> indices({
                                                    tv.emplace(1),
                                                    tv.emplace(2),
                                                    tv.emplace(3),
                                                    tv.emplace(4),
                                                });

    TestVector::iterator last;
    for (std::size_t i = 4; i < TestVector::BLOCK_SIZE+1; ++i) {
        last = tv.emplace(i+1);
    }

    CHECK(!tv.empty());
    CHECK(tv.size() == TestVector::BLOCK_SIZE+1);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE*2);
    CHECK(tv.blocks() == 2);
    CHECK(tv.regions() == 2);
    tv.erase(indices[1]);
    tv.erase(indices[3]);
    tv.erase(last);

    std::vector<NonTriviallyConstructable> from1(tv.begin(), tv.end());

    NonTriviallyConstructable::recorded_events.clear();

    tv.trim();
    CHECK(!tv.empty());
    CHECK(tv.size() == TestVector::BLOCK_SIZE-2);
    CHECK(tv.capacity() == TestVector::BLOCK_SIZE);
    CHECK(tv.blocks() == 1);
    CHECK(tv.regions() == 5);

    std::vector<NonTriviallyConstructable::Event> ref({
                                                      });
    CHECK(NonTriviallyConstructable::recorded_events == ref);

    std::vector<NonTriviallyConstructable> from2(tv.begin(), tv.end());
    CHECK(from1 == from2);
}
