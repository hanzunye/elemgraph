// Checks the iterative dfs() against a recursive DFS written directly from the
// edge list, on random multigraphs.
#include "elemgraph/algorithms/dfs.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <vector>

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/csr_graph.hpp"
#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"

namespace elemgraph::test {
namespace {

// The textbook recursive DFS. It visits the neighbors of each vertex in the
// order the library promises: by edge id, without self-loops. Fine for the
// small graphs used here; recursion depth is at most n.
struct recursive_dfs {
    std::vector<vertex> order;
    std::vector<vertex> parent;

    explicit recursive_dfs(const input_graph& g) : parent(g.n, no_vertex) {
        std::vector<std::vector<vertex>> neighbors(g.n);
        for (const auto [u, v] : g.edges) {
            if (u != v) {
                neighbors[u].push_back(v);
                neighbors[v].push_back(u);
            }
        }
        std::vector<bool> reached(g.n, false);
        const std::function<void(vertex)> visit = [&](vertex v) {
            reached[v] = true;
            order.push_back(v);
            for (const vertex w : neighbors[v]) {
                if (!reached[w]) {
                    parent[w] = v;
                    visit(w);
                }
            }
        };
        for (vertex root = 0; root < g.n; ++root) {
            if (!reached[root]) {
                visit(root);
            }
        }
    }
};

void expect_same_search(const dfs_result& result, const recursive_dfs& expected) {
    EXPECT_EQ(result.order, expected.order);
    EXPECT_EQ(result.parent, expected.parent);
    ASSERT_EQ(result.number.size(), expected.order.size());
    for (vertex i = 0; i < expected.order.size(); ++i) {
        EXPECT_EQ(result.number[expected.order[i]], i);
    }
}

TEST(DfsRandom, MatchesARecursiveDfsOnBothRepresentations) {
    for_each_random_graph(1, 1000, small_random_multigraph, [](const input_graph& g) {
        const edge_list list = to_edge_list(g);
        const recursive_dfs expected(g);
        {
            SCOPED_TRACE("csr_graph");
            expect_same_search(dfs(csr_graph(list)), expected);
        }
        {
            SCOPED_TRACE("adjacency_list");
            expect_same_search(dfs(adjacency_list(list)), expected);
        }
    });
}

// The property that lowpoints and chains rely on: in a DFS of an undirected
// graph every edge joins a vertex and one of its ancestors (there are no
// cross edges).
TEST(DfsRandom, EveryEdgeJoinsAVertexAndAnAncestor) {
    for_each_random_graph(1001, 1000, small_random_multigraph, [](const input_graph& g) {
        const dfs_result result = dfs(csr_graph(to_edge_list(g)));
        const auto is_ancestor = [&](vertex a, vertex v) {
            for (; v != no_vertex; v = result.parent[v]) {
                if (v == a) {
                    return true;
                }
            }
            return false;
        };
        for (const auto [u, v] : g.edges) {
            EXPECT_TRUE(is_ancestor(u, v) || is_ancestor(v, u)) << "edge " << u << "-" << v;
        }
    });
}

}  // namespace
}  // namespace elemgraph::test
