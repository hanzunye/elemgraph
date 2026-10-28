#include "random_graphs.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

#include "differential.hpp"
#include "reference.hpp"

namespace elemgraph::test {
namespace {

// The same seed must give the same numbers with libstdc++ (Linux) and libc++
// (macOS); CI runs this test on both.
TEST(Rng, GivesTheSameNumbersOnEveryPlatform) {
    rng r(42);
    std::vector<std::uint64_t> numbers;
    for (int i = 0; i < 5; ++i) {
        numbers.push_back(r.below(1000));
    }
    EXPECT_EQ(numbers, (std::vector<std::uint64_t>{406, 824, 450, 662, 381}));

    std::vector<int> items{0, 1, 2, 3, 4, 5, 6, 7};
    r.shuffle(items);
    EXPECT_EQ(items, (std::vector<int>{2, 6, 3, 1, 5, 0, 7, 4}));
}

TEST(Rng, BelowStaysInRangeAndHitsEveryValue) {
    rng r(1);
    for (const std::uint64_t bound : {1u, 2u, 3u, 7u}) {
        std::set<std::uint64_t> seen;
        for (int i = 0; i < 1000; ++i) {
            const std::uint64_t x = r.below(bound);
            ASSERT_LT(x, bound);
            seen.insert(x);
        }
        EXPECT_EQ(seen.size(), bound);
    }
}

TEST(RandomSimpleGraph, HasNoSelfLoopsOrParallelEdges) {
    rng r(7);
    const input_graph g = random_simple_graph(r, 30, 200);
    EXPECT_EQ(g.n, 30u);
    ASSERT_EQ(g.edges.size(), 200u);
    std::set<std::pair<vertex, vertex>> seen;
    for (const edge e : g.edges) {
        EXPECT_LT(e.u, g.n);
        EXPECT_LT(e.v, g.n);
        EXPECT_NE(e.u, e.v) << "self-loop " << e;
        EXPECT_TRUE(seen.insert(std::minmax(e.u, e.v)).second) << "parallel edge " << e;
    }
}

TEST(RandomSimpleGraph, CanBeComplete) {
    rng r(7);
    EXPECT_EQ(random_simple_graph(r, 10, 45).edges.size(), 45u);
}

TEST(RandomMultigraph, HasSelfLoopsAndParallelEdgesInBothDirections) {
    rng r(7);
    const input_graph g = random_multigraph(r, 30, 300);
    ASSERT_EQ(g.edges.size(), 300u);
    int self_loops = 0;
    int same_direction = 0;
    int reversed = 0;
    std::set<edge> seen;
    for (const edge e : g.edges) {
        EXPECT_LT(e.u, g.n);
        EXPECT_LT(e.v, g.n);
        if (e.u == e.v) {
            ++self_loops;
            continue;
        }
        same_direction += seen.contains(e) ? 1 : 0;
        reversed += seen.contains({e.v, e.u}) ? 1 : 0;
        seen.insert(e);
    }
    EXPECT_GT(self_loops, 0);
    EXPECT_GT(same_direction, 0);
    EXPECT_GT(reversed, 0);
}

TEST(RandomMultigraph, OneVertexGivesOnlySelfLoops) {
    rng r(7);
    const input_graph g = random_multigraph(r, 1, 5);
    EXPECT_EQ(g.edges, std::vector<edge>(5, edge{0, 0}));
}

TEST(RandomTree, IsConnectedWithNMinusOneEdges) {
    const auto make = [](rng& r) { return random_tree(r, static_cast<vertex>(1 + r.below(50))); };
    for_each_random_graph(0, 200, make, [](const input_graph& g) {
        EXPECT_EQ(g.edges.size(), g.n - 1);
        EXPECT_EQ(connected_components(g), std::vector<std::uint32_t>(g.n, 0));
    });
}

TEST(RandomGraphs, SameSeedGivesSameGraph) {
    rng a(123);
    rng b(123);
    EXPECT_EQ(small_random_multigraph(a), small_random_multigraph(b));
    EXPECT_EQ(small_random_simple_graph(a), small_random_simple_graph(b));
}

}  // namespace
}  // namespace elemgraph::test
