// Breadth-first search from one source: distances and a shortest-path tree.
//
// Two versions that differ only in the queue (DESIGN.md §4.4):
//   bfs()            a flat array of n entries with a head and a tail index.
//                    Every vertex enters the queue at most once, so n entries
//                    are enough, and with a reused workspace nothing is
//                    allocated during the search.
//   bfs_std_queue()  std::queue, i.e. a std::deque, which allocates blocks as
//                    it grows: the baseline for the flat version.
// Both visit the vertices in the same order and give the same result.
//
// They work on any StaticGraph (concepts.hpp), such as csr_graph and
// adjacency_list.
#pragma once

#include <cassert>
#include <cstddef>
#include <limits>
#include <queue>
#include <vector>

#include "../concepts.hpp"
#include "../edge_list.hpp"

namespace elemgraph {

// The distance of a vertex that the search does not reach.
inline constexpr vertex unreachable = std::numeric_limits<vertex>::max();

struct bfs_result {
    // distance[v]: the number of edges on a shortest path from the source to
    // v, or unreachable.
    std::vector<vertex> distance;
    // parent[v]: the vertex from which the search reached v, so the vertex
    // before v on a shortest path; no_vertex for the source and for vertices
    // that are not reached.
    std::vector<vertex> parent;
};

// Memory that bfs() reuses from one call to the next, so that repeated
// searches, as in benchmarks, do not allocate. Holds nothing of interest
// between calls.
struct bfs_workspace {
    std::vector<vertex> queue;
};

// Searches g from `source` and overwrites `result`. The queue is a flat array.
template <StaticGraph Graph>
void bfs(const Graph& g, vertex source, bfs_result& result, bfs_workspace& workspace) {
    const vertex n = g.num_vertices();
    assert(source < n);
    result.distance.assign(n, unreachable);
    result.parent.assign(n, no_vertex);

    // The vertices waiting to be scanned are queue[head .. tail-1]. Both
    // indices only move forward, and tail grows at most n times.
    std::vector<vertex>& queue = workspace.queue;
    queue.resize(n);
    std::size_t head = 0;
    std::size_t tail = 0;

    result.distance[source] = 0;
    queue[tail++] = source;
    while (head < tail) {
        const vertex u = queue[head++];
        const vertex next_distance = result.distance[u] + 1;
        for (const vertex w : g.neighbors(u)) {
            if (result.distance[w] == unreachable) {
                result.distance[w] = next_distance;
                result.parent[w] = u;
                queue[tail++] = w;
            }
        }
    }
}

// The same search with std::queue as the queue.
template <StaticGraph Graph>
void bfs_std_queue(const Graph& g, vertex source, bfs_result& result) {
    const vertex n = g.num_vertices();
    assert(source < n);
    result.distance.assign(n, unreachable);
    result.parent.assign(n, no_vertex);

    std::queue<vertex> queue;
    result.distance[source] = 0;
    queue.push(source);
    while (!queue.empty()) {
        const vertex u = queue.front();
        queue.pop();
        const vertex next_distance = result.distance[u] + 1;
        for (const vertex w : g.neighbors(u)) {
            if (result.distance[w] == unreachable) {
                result.distance[w] = next_distance;
                result.parent[w] = u;
                queue.push(w);
            }
        }
    }
}

// Convenience versions that allocate their own result.
template <StaticGraph Graph>
[[nodiscard]] bfs_result bfs(const Graph& g, vertex source) {
    bfs_result result;
    bfs_workspace workspace;
    bfs(g, source, result, workspace);
    return result;
}

template <StaticGraph Graph>
[[nodiscard]] bfs_result bfs_std_queue(const Graph& g, vertex source) {
    bfs_result result;
    bfs_std_queue(g, source, result);
    return result;
}

}  // namespace elemgraph
