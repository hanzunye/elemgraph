#include "elemgraph/algorithms/union_find.hpp"

#include <gtest/gtest.h>

namespace elemgraph {
namespace {

TEST(UnionFind, StartsWithOneSetPerVertex) {
    union_find sets(4);
    for (vertex v = 0; v < 4; ++v) {
        EXPECT_EQ(sets.find(v), v);
    }
}

TEST(UnionFind, UniteMergesSetsOnce) {
    union_find sets(5);
    EXPECT_TRUE(sets.unite(0, 1));
    EXPECT_TRUE(sets.unite(3, 4));
    EXPECT_TRUE(sets.unite(1, 4));
    EXPECT_FALSE(sets.unite(0, 3));  // already one set
    EXPECT_EQ(sets.find(0), sets.find(3));
    EXPECT_NE(sets.find(0), sets.find(2));
}

TEST(UnionFind, ResetForgetsEverything) {
    union_find sets(3);
    sets.unite(0, 1);
    sets.reset(2);
    EXPECT_EQ(sets.find(0), 0u);
    EXPECT_EQ(sets.find(1), 1u);
}

TEST(UnionFind, LongChainOfUnionsEndsInOneSet) {
    const vertex n = 100'000;
    union_find sets(n);
    for (vertex v = 0; v + 1 < n; ++v) {
        sets.unite(v + 1, v);
    }
    const vertex root = sets.find(0);
    for (vertex v = 0; v < n; ++v) {
        ASSERT_EQ(sets.find(v), root) << "vertex " << v;
    }
}

}  // namespace
}  // namespace elemgraph
