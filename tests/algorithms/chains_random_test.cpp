// Checks the chain decomposition on random graphs: its bridges and cut
// vertices against the brute-force references and against lowpoints, and the
// chains themselves against the properties in [Schmidt 2013].
#include "elemgraph/algorithms/connectivity.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include "elemgraph/csr_graph.hpp"
#include "support/convert.hpp"
#include "support/differential.hpp"
#include "support/random_graphs.hpp"
#include "support/reference.hpp"

namespace elemgraph::test {
namespace {

void expect_reference_and_lowpoint(const input_graph& g) {
    const csr_graph csr(to_edge_list(g));
    const dfs_result search = dfs(csr);
    connectivity_result chains;
    chain_workspace chain_memory;
    chain_connectivity(csr, search, chains, chain_memory);
    connectivity_result lowpoints;
    lowpoint_workspace lowpoint_memory;
    lowpoint_connectivity(csr, search, lowpoints, lowpoint_memory);

    // Same DFS, same order of output: the two versions must agree exactly.
    EXPECT_EQ(chains.bridges, lowpoints.bridges);
    EXPECT_EQ(chains.cut_vertices, lowpoints.cut_vertices);
    std::vector<edge_id> sorted = chains.bridges;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_EQ(sorted, bridges(g));
    EXPECT_EQ(chains.cut_vertices, cut_vertices(g));
}

TEST(ChainsRandom, MatchReferenceAndLowpointOnMultigraphs) {
    for_each_random_graph(1, 1000, small_random_multigraph, expect_reference_and_lowpoint);
}

TEST(ChainsRandom, MatchReferenceAndLowpointOnSimpleGraphs) {
    for_each_random_graph(1001, 1000, small_random_simple_graph, expect_reference_and_lowpoint);
}

TEST(ChainsRandom, MatchReferenceAndLowpointOnTrees) {
    const auto make = [](rng& r) { return random_tree(r, static_cast<vertex>(r.below(40) + 1)); };
    for_each_random_graph(2001, 200, make, expect_reference_and_lowpoint);
}

// Every edge except the bridges and self-loops lies in exactly one chain;
// consecutive vertices of a chain are the ends of the edge between them;
// chains are found in increasing DFS number of their first vertex; and the
// first chain of every DFS tree is a cycle.
TEST(ChainsRandom, ChainsPartitionTheEdgesThatAreNoBridges) {
    for_each_random_graph(3001, 1000, small_random_multigraph, [](const input_graph& g) {
        const csr_graph csr(to_edge_list(g));
        const dfs_result search = dfs(csr);
        connectivity_result result;
        chain_workspace workspace;
        chain_decomposition chains;
        chain_connectivity(csr, search, result, workspace, chains);

        std::vector<int> times_in_a_chain(g.edges.size(), 0);
        vertex previous_tree_root = no_vertex;
        vertex previous_start = 0;
        for (std::size_t i = 0; i < chains.size(); ++i) {
            const auto vs = chains.vertices(i);
            const auto es = chains.edges(i);
            ASSERT_EQ(es.size() + 1, vs.size()) << "chain " << i;
            for (std::size_t j = 0; j < es.size(); ++j) {
                const auto [a, b] = g.edges[es[j]];
                EXPECT_TRUE((a == vs[j] && b == vs[j + 1]) || (b == vs[j] && a == vs[j + 1]))
                    << "chain " << i << ", edge " << es[j];
                ++times_in_a_chain[es[j]];
            }
            EXPECT_GE(search.number[vs[0]], search.number[previous_start]) << "chain " << i;
            previous_start = vs[0];

            vertex root = vs[0];
            while (search.parent[root] != no_vertex) {
                root = search.parent[root];
            }
            if (root != previous_tree_root) {
                EXPECT_TRUE(chains.is_cycle(i)) << "chain " << i << " is the first of its tree";
                previous_tree_root = root;
            }
        }

        std::vector<bool> is_bridge(g.edges.size(), false);
        for (const edge_id e : result.bridges) {
            is_bridge[e] = true;
        }
        for (edge_id e = 0; e < g.edges.size(); ++e) {
            const auto [a, b] = g.edges[e];
            const int expected = (a == b || is_bridge[e]) ? 0 : 1;
            EXPECT_EQ(times_in_a_chain[e], expected) << "edge " << e << " = " << g.edges[e];
        }
    });
}

}  // namespace
}  // namespace elemgraph::test
