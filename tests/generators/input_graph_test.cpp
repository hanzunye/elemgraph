#include "generators/input_graph.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace elemgraph::gen {
namespace {

TEST(Canonicalize, OrientsSortsAndRemovesSelfLoopsAndRepeats) {
    input_graph g{4, {{2, 1}, {0, 3}, {1, 2}, {3, 3}, {3, 0}, {0, 1}}};
    canonicalize(g);
    EXPECT_EQ(g.edges, (std::vector<edge>{{0, 1}, {0, 3}, {1, 2}}));
    EXPECT_EQ(g.n, 4u);
    EXPECT_TRUE(is_canonical(g));
}

TEST(Canonicalize, EmptyGraph) {
    input_graph g;
    canonicalize(g);
    EXPECT_TRUE(g.edges.empty());
    EXPECT_TRUE(is_canonical(g));
}

TEST(IsCanonical, RejectsEachViolation) {
    EXPECT_TRUE(is_canonical({3, {{0, 1}, {0, 2}, {1, 2}}}));
    EXPECT_FALSE(is_canonical({3, {{1, 0}}})) << "reversed";
    EXPECT_FALSE(is_canonical({3, {{1, 1}}})) << "self-loop";
    EXPECT_FALSE(is_canonical({3, {{0, 1}, {0, 1}}})) << "repeated";
    EXPECT_FALSE(is_canonical({3, {{0, 2}, {0, 1}}})) << "unsorted";
    EXPECT_FALSE(is_canonical({3, {{0, 3}}})) << "vertex out of range";
}

}  // namespace
}  // namespace elemgraph::gen
