// Bridges and cut vertices (DESIGN.md §4.4), computed from a DFS forest in
// two ways that give identical results:
//
//   lowpoint_connectivity()  the classic approach [Hopcroft & Tarjan 1973;
//       Tarjan 1974]: low[v] is the smallest DFS number reachable from the
//       subtree of v by tree edges down and then one non-tree edge.
//   chain_connectivity()     the chain decomposition [Schmidt 2013]; it can
//       also return the chains themselves. docs/chain-decomposition.md proves
//       its rules for graphs with parallel edges, several components and
//       vertices of degree 1, which the paper does not cover.
//
// Both start from the same dfs() result, so a benchmark can time the search
// and the rest separately (Q4).
//
// Parallel edges are kept: of two edges between the same vertices, neither is
// a bridge. Self-loops do not matter; csr_graph has already dropped them.
#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
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

// The chains of a chain decomposition, in the order in which they were found.
// vertices(i) lists the vertices of chain i in the order of the traversal:
// its first vertex v, the other end of the back edge from v, then upwards in
// the DFS tree. edges(i)[j] is the edge between vertices(i)[j] and
// vertices(i)[j + 1]. A chain is a cycle iff its last vertex is its first.
class chain_decomposition {
public:
    [[nodiscard]] std::size_t size() const { return begin_.size() - 1; }

    [[nodiscard]] std::span<const vertex> vertices(std::size_t i) const {
        return std::span<const vertex>(vertices_).subspan(begin_[i], begin_[i + 1] - begin_[i]);
    }

    [[nodiscard]] std::span<const edge_id> edges(std::size_t i) const {
        // A chain has one edge fewer than vertices, so the edges of chain i
        // start i positions before its vertices.
        return std::span<const edge_id>(edges_).subspan(begin_[i] - i,
                                                        begin_[i + 1] - begin_[i] - 1);
    }

    [[nodiscard]] bool is_cycle(std::size_t i) const {
        const auto vs = vertices(i);
        return vs.front() == vs.back();
    }

    // Used by chain_connectivity() while it builds the chains.
    void clear() {
        begin_.assign(1, 0);
        vertices_.clear();
        edges_.clear();
    }
    void start(vertex v) { vertices_.push_back(v); }
    void extend(edge_id e, vertex w) {
        edges_.push_back(e);
        vertices_.push_back(w);
    }
    void finish() { begin_.push_back(vertices_.size()); }

private:
    std::vector<std::size_t> begin_{0};  // vertices of chain i: begin_[i] .. begin_[i + 1] - 1
    std::vector<vertex> vertices_;
    std::vector<edge_id> edges_;
};

// Memory that chain_connectivity() reuses from one call to the next.
struct chain_workspace {
    // state[v]: unvisited, the first vertex of a chain, or climbed, i.e. a
    // chain went up through the tree edge from v to its parent.
    std::vector<std::uint8_t> state;
    // tree_edge_seen[w]: the scan of parent[w]'s edges has passed the tree
    // edge to w, so further edges from parent[w] to w are back edges.
    std::vector<std::uint8_t> tree_edge_seen;
    std::vector<std::uint8_t> is_cut;
};

namespace detail {

inline constexpr std::uint8_t unvisited = 0;
inline constexpr std::uint8_t chain_start = 1;
inline constexpr std::uint8_t climbed = 2;

// The id of the tree edge between x and its parent: the first edge to the
// parent in x's list (see dfs.hpp).
template <EdgeIndexedGraph Graph>
edge_id tree_edge(const Graph& g, const dfs_result& search, vertex x) {
    const vertex p = search.parent[x];
    const auto neighbors = g.neighbors(x);
    const auto ids = g.edge_ids(x);
    std::size_t j = 0;
    while (neighbors[j] != p) {
        ++j;
    }
    return ids[j];
}

// The chain decomposition and what follows from it. With record == false,
// `chains` is not touched, so benchmarks do not pay for storing the chains.
template <bool record, EdgeIndexedGraph Graph>
void chain_connectivity(const Graph& g, const dfs_result& search, connectivity_result& result,
                        chain_workspace& workspace, chain_decomposition* chains) {
    const vertex n = g.num_vertices();
    assert(search.order.size() == n);
    std::vector<std::uint8_t>& state = workspace.state;
    state.assign(n, unvisited);
    std::vector<std::uint8_t>& tree_edge_seen = workspace.tree_edge_seen;
    tree_edge_seen.assign(n, 0);
    std::vector<std::uint8_t>& is_cut = workspace.is_cut;
    is_cut.assign(n, 0);
    result.bridges.clear();
    result.cut_vertices.clear();
    if constexpr (record) {
        chains->clear();
    }

    // The vertices in increasing DFS number, and at each vertex v every back
    // edge from v down to a descendant w: each gives one chain, which follows
    // the back edge and then tree edges upwards until it meets a vertex that
    // an earlier chain (or this chain's start) has visited.
    bool tree_has_chain = false;  // whether the current DFS tree has a chain yet
    for (const vertex v : search.order) {
        if (search.parent[v] == no_vertex) {
            tree_has_chain = false;  // v is the root of the next tree
        }
        const auto neighbors = g.neighbors(v);
        const auto ids = g.edge_ids(v);
        for (std::size_t j = 0; j < neighbors.size(); ++j) {
            const vertex w = neighbors[j];
            // No cross edges in a DFS: a larger DFS number means a descendant.
            if (search.number[w] <= search.number[v]) {
                continue;
            }
            if (search.parent[w] == v && tree_edge_seen[w] == 0) {
                tree_edge_seen[w] = 1;  // the tree edge to the child w
                continue;
            }

            if (state[v] == unvisited) {
                state[v] = chain_start;
            }
            if constexpr (record) {
                chains->start(v);
                chains->extend(ids[j], w);
            }
            vertex x = w;
            while (state[x] == unvisited) {
                state[x] = climbed;
                const vertex p = search.parent[x];
                if constexpr (record) {
                    chains->extend(tree_edge(g, search, x), p);
                }
                x = p;
            }
            if constexpr (record) {
                chains->finish();
            }

            // The first chain of a tree is its C_1. Every later chain that
            // is a cycle starts at a cut vertex [Schmidt 2013].
            if (!tree_has_chain) {
                tree_has_chain = true;
            } else if (x == v) {
                is_cut[v] = 1;
            }
        }
    }

    // A tree edge is a bridge iff no chain contains it, i.e. iff no chain
    // climbed from its lower end. An end of a bridge is a cut vertex iff it
    // has another edge.
    for (const vertex x : search.order) {
        const vertex p = search.parent[x];
        if (p == no_vertex || state[x] == climbed) {
            continue;
        }
        result.bridges.push_back(tree_edge(g, search, x));
        if (g.degree(x) >= 2) {
            is_cut[x] = 1;
        }
        if (g.degree(p) >= 2) {
            is_cut[p] = 1;
        }
    }
    for (vertex v = 0; v < n; ++v) {
        if (is_cut[v] != 0) {
            result.cut_vertices.push_back(v);
        }
    }
}

}  // namespace detail

// Overwrites `result`. `search` must be dfs() of g.
template <EdgeIndexedGraph Graph>
void chain_connectivity(const Graph& g, const dfs_result& search, connectivity_result& result,
                        chain_workspace& workspace) {
    detail::chain_connectivity<false>(g, search, result, workspace, nullptr);
}

// The same, and also overwrites `chains` with the chain decomposition.
template <EdgeIndexedGraph Graph>
void chain_connectivity(const Graph& g, const dfs_result& search, connectivity_result& result,
                        chain_workspace& workspace, chain_decomposition& chains) {
    detail::chain_connectivity<true>(g, search, result, workspace, &chains);
}

// Convenience version: runs dfs() too and allocates its own memory.
template <EdgeIndexedGraph Graph>
[[nodiscard]] connectivity_result chain_connectivity(const Graph& g) {
    const dfs_result search = dfs(g);
    connectivity_result result;
    chain_workspace workspace;
    chain_connectivity(g, search, result, workspace);
    return result;
}

}  // namespace elemgraph
