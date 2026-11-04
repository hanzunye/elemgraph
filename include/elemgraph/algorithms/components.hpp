// Connected components, in two versions (DESIGN.md §4.4):
//   connected_components_bfs()         a BFS from every vertex not yet reached,
//                                      O(n + m).
//   connected_components_union_find()  unites the ends of every edge,
//                                      O((n + m) α(n)).
// Both number the components 0, 1, ... in the order of their smallest vertex,
// so for the same graph they give the same result.
#pragma once

#include <cstddef>
#include <vector>

#include "../concepts.hpp"
#include "../edge_list.hpp"
#include "bfs.hpp"
#include "union_find.hpp"

namespace elemgraph {

struct components_result {
    vertex count = 0;  // the number of components
    // component[v]: the number of the component that contains v, 0 .. count-1.
    std::vector<vertex> component;
};

// Overwrites `result`. The BFS queue comes from `workspace`, as for bfs().
template <StaticGraph Graph>
void connected_components_bfs(const Graph& g, components_result& result,
                              bfs_workspace& workspace) {
    const vertex n = g.num_vertices();
    result.count = 0;
    result.component.assign(n, no_vertex);  // no_vertex: not reached yet

    // Each vertex enters the queue once over all searches, so n entries are
    // enough; every search starts again at the front.
    std::vector<vertex>& queue = workspace.queue;
    queue.resize(n);
    for (vertex s = 0; s < n; ++s) {
        if (result.component[s] != no_vertex) {
            continue;
        }
        const vertex c = result.count++;
        std::size_t head = 0;
        std::size_t tail = 0;
        result.component[s] = c;
        queue[tail++] = s;
        while (head < tail) {
            const vertex u = queue[head++];
            for (const vertex w : g.neighbors(u)) {
                if (result.component[w] == no_vertex) {
                    result.component[w] = c;
                    queue[tail++] = w;
                }
            }
        }
    }
}

// Overwrites `result`; `sets` is reset and reused.
template <StaticGraph Graph>
void connected_components_union_find(const Graph& g, components_result& result,
                                     union_find& sets) {
    const vertex n = g.num_vertices();
    sets.reset(n);
    for (vertex u = 0; u < n; ++u) {
        for (const vertex w : g.neighbors(u)) {
            if (u < w) {  // every edge is stored at both ends; unite once
                sets.unite(u, w);
            }
        }
    }

    // Number the sets in the order of their smallest vertex. component[r] of
    // a representative r holds the number of its set as soon as the set has
    // one; r itself may come later in the loop and then finds it there.
    result.count = 0;
    result.component.assign(n, no_vertex);
    for (vertex v = 0; v < n; ++v) {
        const vertex r = sets.find(v);
        if (result.component[r] == no_vertex) {
            result.component[r] = result.count++;
        }
        result.component[v] = result.component[r];
    }
}

// Convenience versions that allocate their own memory.
template <StaticGraph Graph>
[[nodiscard]] components_result connected_components_bfs(const Graph& g) {
    components_result result;
    bfs_workspace workspace;
    connected_components_bfs(g, result, workspace);
    return result;
}

template <StaticGraph Graph>
[[nodiscard]] components_result connected_components_union_find(const Graph& g) {
    components_result result;
    union_find sets;
    connected_components_union_find(g, result, sets);
    return result;
}

}  // namespace elemgraph
