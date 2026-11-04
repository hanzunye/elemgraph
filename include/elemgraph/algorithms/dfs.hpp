// Depth-first search over the whole graph, the core that the connectivity
// algorithms build on (DESIGN.md §4.4). It is iterative, with an explicit
// stack of (vertex, index of the next neighbor), so that a deep graph, such as
// a path with millions of vertices, cannot overflow the call stack.
//
// Roots are tried in increasing id and the neighbors of each vertex in the
// order of neighbors(v), so the result depends only on the graph. It holds the
// DFS numbers, the parents and the preorder; lowpoints and chains are computed
// from these afterwards, so the search needs no callbacks.
//
// Parallel edges: with csr_graph, neighbors(v) is ordered by edge id. The
// search goes from u to a new vertex w through the first of the edges between
// them in u's list, which is the one with the smallest id; in w's list, the
// first edge to u is the same edge. So the tree edge between w and parent[w] is
// the first edge to parent[w] in w's list, and the search need not store it.
#pragma once

#include <cstddef>
#include <vector>

#include "../concepts.hpp"
#include "../edge_list.hpp"

namespace elemgraph {

struct dfs_result {
    // All vertices in preorder, the order in which the search reached them.
    std::vector<vertex> order;
    // number[v]: the position of v in order, its DFS number.
    std::vector<vertex> number;
    // parent[v]: the vertex from which the search reached v; no_vertex for
    // the root of each tree, which is the smallest vertex of its component.
    std::vector<vertex> parent;
};

// Memory that dfs() reuses from one call to the next. Holds nothing of
// interest between calls.
struct dfs_workspace {
    struct frame {
        vertex v;
        std::size_t next;  // the index in neighbors(v) of the next edge to follow
    };
    std::vector<frame> stack;
};

// Searches all of g and overwrites `result`.
template <StaticGraph Graph>
void dfs(const Graph& g, dfs_result& result, dfs_workspace& workspace) {
    const vertex n = g.num_vertices();
    result.order.clear();
    result.order.reserve(n);
    result.number.assign(n, no_vertex);  // no_vertex: not reached yet
    result.parent.assign(n, no_vertex);

    // The stack holds the path from the root to the current vertex, so at
    // most n frames. Reserving them up front means push_back never moves the
    // frames, which the loop below relies on.
    std::vector<dfs_workspace::frame>& stack = workspace.stack;
    stack.clear();
    stack.reserve(n);

    const auto reach = [&](vertex v, vertex parent) {
        result.number[v] = static_cast<vertex>(result.order.size());
        result.order.push_back(v);
        result.parent[v] = parent;
        stack.push_back({v, 0});
    };

    for (vertex root = 0; root < n; ++root) {
        if (result.number[root] != no_vertex) {
            continue;
        }
        reach(root, no_vertex);
        while (!stack.empty()) {
            dfs_workspace::frame& top = stack.back();
            const auto neighbors = g.neighbors(top.v);
            if (top.next == neighbors.size()) {
                stack.pop_back();  // all edges of top.v followed: back to its parent
                continue;
            }
            const vertex w = neighbors[top.next];
            ++top.next;
            if (result.number[w] == no_vertex) {
                reach(w, top.v);
            }
        }
    }
}

// Convenience version that allocates its own result.
template <StaticGraph Graph>
[[nodiscard]] dfs_result dfs(const Graph& g) {
    dfs_result result;
    dfs_workspace workspace;
    dfs(g, result, workspace);
    return result;
}

}  // namespace elemgraph
