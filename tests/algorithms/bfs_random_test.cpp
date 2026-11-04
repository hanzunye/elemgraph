// Checks both BFS versions on both representations against the brute-force
// distances, on random multigraphs and from every source.
#include "elemgraph/algorithms/bfs.hpp"

#include <gtest/gtest.h>

#include <algorithm>

#include "elemgraph/adjacency_list.hpp"
#include "elemgraph/csr_graph.hpp"
#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"
#include "support/reference.hpp"

namespace elemgraph::test {
namespace {

// The distances are right, and parent[] is a shortest-path tree of g: the
// parent of every reached vertex other than the source is a neighbor one step
// closer to the source.
void expect_shortest_path_tree(const csr_graph& g, const input_graph& input, vertex source,
                               const bfs_result& result) {
    EXPECT_EQ(result.distance, distances(input, source));
    ASSERT_EQ(result.parent.size(), g.num_vertices());
    for (vertex v = 0; v < g.num_vertices(); ++v) {
        const vertex p = result.parent[v];
        if (v == source || result.distance[v] == unreachable) {
            EXPECT_EQ(p, no_vertex) << "vertex " << v;
            continue;
        }
        ASSERT_LT(p, g.num_vertices()) << "vertex " << v;
        EXPECT_EQ(result.distance[p] + 1, result.distance[v]) << "vertex " << v;
        const auto nbrs = g.neighbors(v);
        EXPECT_NE(std::find(nbrs.begin(), nbrs.end(), p), nbrs.end())
            << "parent " << p << " is not a neighbor of " << v;
    }
}

TEST(BfsRandom, AllVersionsFindShortestPathTreesAndAgree) {
    for_each_random_graph(1, 500, small_random_multigraph, [](const input_graph& g) {
        const edge_list list = to_edge_list(g);
        const csr_graph csr(list);
        const adjacency_list adj(list);
        bfs_result result;
        bfs_workspace workspace;
        for (vertex source = 0; source < g.n; ++source) {
            SCOPED_TRACE(::testing::Message() << "source " << source);
            bfs(csr, source, result, workspace);  // reused, as in benchmarks
            expect_shortest_path_tree(csr, g, source, result);

            // The neighbors come in the same order everywhere, so the other
            // versions must give exactly the same tree.
            const bfs_result std_queue = bfs_std_queue(csr, source);
            EXPECT_EQ(std_queue.distance, result.distance);
            EXPECT_EQ(std_queue.parent, result.parent);
            const bfs_result on_adj = bfs(adj, source);
            EXPECT_EQ(on_adj.distance, result.distance);
            EXPECT_EQ(on_adj.parent, result.parent);
            const bfs_result std_queue_on_adj = bfs_std_queue(adj, source);
            EXPECT_EQ(std_queue_on_adj.distance, result.distance);
            EXPECT_EQ(std_queue_on_adj.parent, result.parent);
            if (::testing::Test::HasFailure()) {
                return;
            }
        }
    });
}

}  // namespace
}  // namespace elemgraph::test
