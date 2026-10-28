// Brute-force references (test oracles). Each one follows the textbook
// definition as directly as possible and makes no attempt to be fast:
//   connected_components, cut_vertices   O(n (n + m))
//   distances                            O(n m)
//   bridges                              O(m (n + m))
// so use them on graphs with at most a few hundred vertices.
#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "test_graph.hpp"

namespace elemgraph::test {

// Stands for "no edge", "no vertex" and "unreachable".
inline constexpr std::uint32_t none = std::numeric_limits<std::uint32_t>::max();

namespace detail {

// incidences[v] lists (other endpoint, edge id) for every edge at v.
// A self-loop appears twice at its vertex, which does not matter here.
using incidences = std::vector<std::vector<std::pair<vertex, edge_id>>>;

inline incidences make_incidences(const input_graph& g) {
    incidences inc(g.n);
    for (edge_id e = 0; e < g.edges.size(); ++e) {
        const auto [u, v] = g.edges[e];
        inc[u].push_back({v, e});
        inc[v].push_back({u, e});
    }
    return inc;
}

// reached[x] is true iff x can be reached from `source` without using edge
// `skip_edge` and without entering vertex `skip_vertex`.
inline std::vector<bool> reachable(const incidences& inc, vertex source,
                                   edge_id skip_edge = none, vertex skip_vertex = none) {
    std::vector<bool> reached(inc.size(), false);
    std::vector<vertex> stack{source};
    reached[source] = true;
    while (!stack.empty()) {
        const vertex x = stack.back();
        stack.pop_back();
        for (const auto& [y, e] : inc[x]) {
            if (e == skip_edge || y == skip_vertex || reached[y]) {
                continue;
            }
            reached[y] = true;
            stack.push_back(y);
        }
    }
    return reached;
}

}  // namespace detail

// Component label of every vertex. Components are numbered 0, 1, ... in the
// order of their smallest vertex.
inline std::vector<std::uint32_t> connected_components(const input_graph& g) {
    const auto inc = detail::make_incidences(g);
    std::vector<std::uint32_t> label(g.n, none);
    std::uint32_t count = 0;
    for (vertex s = 0; s < g.n; ++s) {
        if (label[s] != none) {
            continue;
        }
        const auto reached = detail::reachable(inc, s);
        for (vertex x = 0; x < g.n; ++x) {
            if (reached[x]) {
                label[x] = count;
            }
        }
        ++count;
    }
    return label;
}

// Renumbers labels in order of first appearance. Two labelings describe the
// same partition iff their canonical labels are equal, so any labeling (for
// example union-find roots) can be compared with connected_components().
inline std::vector<std::uint32_t> canonical_labels(std::span<const std::uint32_t> labels) {
    std::unordered_map<std::uint32_t, std::uint32_t> renumber;
    std::vector<std::uint32_t> result;
    result.reserve(labels.size());
    for (const std::uint32_t label : labels) {
        const auto next = static_cast<std::uint32_t>(renumber.size());
        result.push_back(renumber.try_emplace(label, next).first->second);
    }
    return result;
}

// Number of edges on a shortest path from `source` to every vertex, `none` for
// unreachable vertices. Computed by relaxing every edge until nothing changes
// (Bellman–Ford with unit weights) rather than by BFS.
inline std::vector<std::uint32_t> distances(const input_graph& g, vertex source) {
    assert(source < g.n);
    std::vector<std::uint32_t> dist(g.n, none);
    dist[source] = 0;
    for (bool changed = true; changed;) {
        changed = false;
        for (const auto& [u, v] : g.edges) {
            if (dist[u] != none && dist[u] + 1 < dist[v]) {
                dist[v] = dist[u] + 1;
                changed = true;
            }
            if (dist[v] != none && dist[v] + 1 < dist[u]) {
                dist[u] = dist[v] + 1;
                changed = true;
            }
        }
    }
    return dist;
}

// Ids of all bridges, ascending. Edge {u, v} is a bridge iff v cannot be
// reached from u once the edge is removed. So self-loops and parallel edges
// are never bridges.
inline std::vector<edge_id> bridges(const input_graph& g) {
    const auto inc = detail::make_incidences(g);
    std::vector<edge_id> result;
    for (edge_id e = 0; e < g.edges.size(); ++e) {
        const auto [u, v] = g.edges[e];
        if (!detail::reachable(inc, u, e)[v]) {
            result.push_back(e);
        }
    }
    return result;
}

// All cut vertices (articulation points), ascending. Vertex v is a cut vertex
// iff removing it disconnects its component, that is, iff some neighbour of v
// can no longer reach another neighbour of v once v is removed. Self-loops
// and parallel edges do not count as extra neighbours.
inline std::vector<vertex> cut_vertices(const input_graph& g) {
    const auto inc = detail::make_incidences(g);
    std::vector<vertex> result;
    for (vertex v = 0; v < g.n; ++v) {
        std::vector<vertex> neighbours;
        for (const auto& [w, e] : inc[v]) {
            if (w != v) {
                neighbours.push_back(w);
            }
        }
        if (neighbours.empty()) {
            continue;
        }
        const auto reached = detail::reachable(inc, neighbours[0], none, v);
        for (const vertex w : neighbours) {
            if (!reached[w]) {
                result.push_back(v);
                break;
            }
        }
    }
    return result;
}

}  // namespace elemgraph::test
