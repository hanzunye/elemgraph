#include "reference.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <vector>

#include "differential.hpp"
#include "random_graphs.hpp"

namespace elemgraph::test {
namespace {

using ::testing::ElementsAre;
using ::testing::IsEmpty;

TEST(Reference, EmptyGraph) {
    const input_graph g{};
    EXPECT_THAT(connected_components(g), IsEmpty());
    EXPECT_THAT(bridges(g), IsEmpty());
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, Path) {
    // 0 - 1 - 2 - 3
    const input_graph g{4, {{0, 1}, {1, 2}, {2, 3}}};
    EXPECT_THAT(connected_components(g), ElementsAre(0u, 0u, 0u, 0u));
    EXPECT_THAT(distances(g, 0), ElementsAre(0u, 1u, 2u, 3u));
    EXPECT_THAT(distances(g, 2), ElementsAre(2u, 1u, 0u, 1u));
    EXPECT_THAT(bridges(g), ElementsAre(0u, 1u, 2u));
    EXPECT_THAT(cut_vertices(g), ElementsAre(1u, 2u));
}

TEST(Reference, Cycle) {
    const input_graph g{4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}}};
    EXPECT_THAT(distances(g, 0), ElementsAre(0u, 1u, 2u, 1u));
    EXPECT_THAT(bridges(g), IsEmpty());
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, Star) {
    const input_graph g{4, {{0, 1}, {2, 0}, {0, 3}}};
    EXPECT_THAT(bridges(g), ElementsAre(0u, 1u, 2u));
    EXPECT_THAT(cut_vertices(g), ElementsAre(0u));
}

TEST(Reference, TwoTrianglesSharingAVertex) {
    // 0 - 1     3 - 4
    //  \ /       \ /
    //   2 ------- 2   (the same vertex)
    const input_graph g{5, {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 2}}};
    EXPECT_THAT(bridges(g), IsEmpty());
    EXPECT_THAT(cut_vertices(g), ElementsAre(2u));
}

TEST(Reference, ParallelEdgesAreNotBridges) {
    // Edges 0 and 1 both join 0 and 1, in opposite directions.
    const input_graph g{3, {{0, 1}, {1, 0}, {1, 2}}};
    EXPECT_THAT(bridges(g), ElementsAre(2u));
    EXPECT_THAT(cut_vertices(g), ElementsAre(1u));
}

TEST(Reference, SelfLoopsChangeNothing) {
    const input_graph g{2, {{0, 0}, {0, 1}, {1, 1}}};
    EXPECT_THAT(connected_components(g), ElementsAre(0u, 0u));
    EXPECT_THAT(distances(g, 0), ElementsAre(0u, 1u));
    EXPECT_THAT(bridges(g), ElementsAre(1u));
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, DisconnectedGraphWithIsolatedVertex) {
    // 0 - 1   2   3 - 4
    const input_graph g{5, {{0, 1}, {3, 4}}};
    EXPECT_THAT(connected_components(g), ElementsAre(0u, 0u, 1u, 2u, 2u));
    EXPECT_THAT(distances(g, 3), ElementsAre(none, none, none, 0u, 1u));
    EXPECT_THAT(bridges(g), ElementsAre(0u, 1u));
    EXPECT_THAT(cut_vertices(g), IsEmpty());
}

TEST(Reference, CanonicalLabels) {
    const std::vector<std::uint32_t> labels{7, 7, 3, 9, 3};
    EXPECT_THAT(canonical_labels(labels), ElementsAre(0u, 0u, 1u, 2u, 1u));
    EXPECT_THAT(canonical_labels({}), IsEmpty());
}

// In a tree every edge is a bridge, and exactly the vertices of degree >= 2
// are cut vertices.
TEST(Reference, Trees) {
    const auto make = [](rng& r) { return random_tree(r, static_cast<vertex>(r.below(40))); };
    for_each_random_graph(0, 300, make, [](const input_graph& g) {
        std::vector<edge_id> all_edges(g.edges.size());
        std::iota(all_edges.begin(), all_edges.end(), edge_id{0});
        EXPECT_EQ(bridges(g), all_edges);

        std::vector<std::uint32_t> degree(g.n, 0);
        for (const edge e : g.edges) {
            ++degree[e.u];
            ++degree[e.v];
        }
        std::vector<vertex> inner;
        for (vertex v = 0; v < g.n; ++v) {
            if (degree[v] >= 2) {
                inner.push_back(v);
            }
        }
        EXPECT_EQ(cut_vertices(g), inner);
    });
}

// Removing a bridge splits one component into two; removing any other edge
// leaves the number of components unchanged.
TEST(Reference, RemovingABridgeAddsOneComponent) {
    const auto component_count = [](const input_graph& g) {
        const auto labels = connected_components(g);
        return labels.empty() ? 0u : *std::max_element(labels.begin(), labels.end()) + 1;
    };
    for_each_random_graph(0, 300, small_random_multigraph, [&](const input_graph& g) {
        const auto bridge_ids = bridges(g);
        const auto before = component_count(g);
        for (edge_id e = 0; e < g.edges.size(); ++e) {
            input_graph without = g;
            without.edges.erase(without.edges.begin() + e);
            const bool is_bridge = std::ranges::binary_search(bridge_ids, e);
            EXPECT_EQ(component_count(without), before + (is_bridge ? 1u : 0u)) << "edge " << e;
        }
    });
}

// The distances satisfy the two conditions that define shortest-path
// distances: the endpoints of an edge differ by at most one, and every reached
// vertex other than the source has a neighbour one step closer.
TEST(Reference, DistancesAreShortestPathDistances) {
    for_each_random_graph(0, 300, small_random_multigraph, [](const input_graph& g) {
        if (g.n == 0) {
            return;
        }
        const auto dist = distances(g, 0);
        EXPECT_EQ(dist[0], 0u);
        std::vector<bool> has_closer_neighbour(g.n, false);
        for (const edge e : g.edges) {
            ASSERT_EQ(dist[e.u] == none, dist[e.v] == none) << "edge " << e;
            if (dist[e.u] == none) {
                continue;
            }
            EXPECT_LE(dist[e.u], dist[e.v] + 1) << "edge " << e;
            EXPECT_LE(dist[e.v], dist[e.u] + 1) << "edge " << e;
            has_closer_neighbour[e.u] = has_closer_neighbour[e.u] || dist[e.v] + 1 == dist[e.u];
            has_closer_neighbour[e.v] = has_closer_neighbour[e.v] || dist[e.u] + 1 == dist[e.v];
        }
        for (vertex v = 1; v < g.n; ++v) {
            if (dist[v] != none) {
                EXPECT_TRUE(has_closer_neighbour[v]) << "vertex " << v;
            }
        }
    });
}

}  // namespace
}  // namespace elemgraph::test
