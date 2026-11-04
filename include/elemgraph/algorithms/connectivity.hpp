// Bridges and cut vertices (DESIGN.md §4.4), computed from a DFS forest.
//
// lowpoint_connectivity() is the classic approach [Hopcroft & Tarjan 1973;
// Tarjan 1974]: low[v] is the smallest DFS number reachable from the subtree
// of v by tree edges down and then one non-tree edge. The chain decomposition
// [Schmidt 2013] will be the second version; both start from the same dfs()
// result, so a benchmark can time the search and the rest separately (Q4).
//
// Parallel edges are kept: of two edges between the same vertices, neither is
// a bridge. Self-loops do not matter; csr_graph has already dropped them.
#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../concepts.hpp"
#include "../edge_list.hpp"
#include "dfs.hpp"

namespace elemgraph {

struct connectivity_result {
    // The ids of all bridges, ordered by the DFS number of their lower end.
    std::vector<edge_id> bridges;
    // All cut vertices (articulation points), ascending.
    std::vector<vertex> cut_vertices;
};

// Memory that lowpoint_connectivity() reuses from one call to the next.
struct lowpoint_workspace {
    std::vector<vertex> low;
    std::vector<std::uint8_t> is_cut;
};

// Overwrites `result`. `search` must be dfs() of g.
template <EdgeIndexedGraph Graph>
void lowpoint_connectivity(const Graph& g, const dfs_result& search, connectivity_result& result,
                           lowpoint_workspace& workspace) {
    const vertex n = g.num_vertices();
    assert(search.order.size() == n);
    std::vector<vertex>& low = workspace.low;
    low.resize(n);
    std::vector<std::uint8_t>& is_cut = workspace.is_cut;
    is_cut.assign(n, 0);
    result.bridges.clear();
    result.cut_vertices.clear();

    // Each tree occupies a contiguous block of the preorder, its root first.
    // Going backwards, the vertices of a tree therefore come right before its
    // root, so a single counter collects the children of the current root.
    vertex root_children = 0;

    // Backwards through the preorder: every child comes before its parent,
    // so low[] of the children is known when the parent is processed.
    for (std::size_t i = n; i-- > 0;) {
        const vertex v = search.order[i];
        const vertex p = search.parent[v];
        const auto neighbors = g.neighbors(v);
        const auto ids = g.edge_ids(v);

        // The tree edge to p is the first edge to p in v's list (see dfs.hpp).
        // Further edges to p are parallel to it, so they count as non-tree
        // edges, and the tree edge is then no bridge.
        bool tree_edge_seen = false;
        edge_id tree_edge = 0;
        vertex low_v = search.number[v];
        for (std::size_t j = 0; j < neighbors.size(); ++j) {
            const vertex w = neighbors[j];
            if (w == p && !tree_edge_seen) {
                tree_edge_seen = true;
                tree_edge = ids[j];
                continue;
            }
            // A child passes on its lowpoint. Any other neighbor is an
            // ancestor or a descendant, since a DFS has no cross edges;
            // only ancestors can lower low_v.
            low_v = std::min(low_v, search.parent[w] == v ? low[w] : search.number[w]);
        }
        low[v] = low_v;

        if (p == no_vertex) {
            // A root is a cut vertex iff it has at least two children.
            if (root_children >= 2) {
                is_cut[v] = 1;
            }
            root_children = 0;
            continue;
        }
        assert(tree_edge_seen);
        // Nothing in the subtree of v reaches p or above without the tree edge.
        if (low_v > search.number[p]) {
            result.bridges.push_back(tree_edge);
        }
        if (search.parent[p] == no_vertex) {
            ++root_children;
        } else if (low_v >= search.number[p]) {
            // Nothing in the subtree of v reaches above p: removing p cuts it off.
            is_cut[p] = 1;
        }
    }

    // Found from the highest DFS number down; reverse to the documented order.
    std::reverse(result.bridges.begin(), result.bridges.end());
    for (vertex v = 0; v < n; ++v) {
        if (is_cut[v] != 0) {
            result.cut_vertices.push_back(v);
        }
    }
}

// Convenience version: runs dfs() too and allocates its own memory.
template <EdgeIndexedGraph Graph>
[[nodiscard]] connectivity_result lowpoint_connectivity(const Graph& g) {
    const dfs_result search = dfs(g);
    connectivity_result result;
    lowpoint_workspace workspace;
    lowpoint_connectivity(g, search, result, workspace);
    return result;
}

}  // namespace elemgraph
