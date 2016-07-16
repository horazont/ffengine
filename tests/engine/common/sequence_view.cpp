#include "ffengine/common/sequence_view.hpp"

#include <catch.hpp>
#include <iostream>

using namespace ffe;

using Vector = std::vector<int>;
using View = SequenceView<Vector>;

TEST_CASE("common/SequenceView<std::vector>")
{
    Vector vec(10);
    View view(vec);

    Vector check(view.begin(), view.end());
    CHECK(vec == check);

    vec[1] = 0;
    CHECK(vec[1] == 0);
    view[1] = 10;
    CHECK(vec[1] == 10);

    CHECK(vec.size() == view.size());
    vec.emplace_back(11);
    CHECK(vec.size() == view.size());
}
