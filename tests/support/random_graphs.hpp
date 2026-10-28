// Seeded random graphs for tests. A seed gives the same graph on every
// platform (see generators/rng.hpp).
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>
#include <set>
#include <utility>
#include <vector>

#include "test_graph.hpp"

namespace elemgraph::test {

// G(n, m): m distinct edges chosen uniformly at random, without self-loops or
// parallel edges. Edges get a random orientation, so both u < v and u > v
// occur. Requires m <= n (n - 1) / 2.
inline input_graph random_simple_graph(rng& r, vertex n, std::uint32_t m) {
    assert(2 * std::uint64_t{m} <= std::uint64_t{n} * (n - 1));
    input_graph g{n, {}};
    g.edges.reserve(m);
    std::set<std::pair<vertex, vertex>> used;  // endpoints as (smaller, larger)
    while (g.edges.size() < m) {
        const auto u = static_cast<vertex>(r.below(n));
        const auto v = static_cast<vertex>(r.below(n));
        if (u != v && used.insert(std::minmax(u, v)).second) {
            g.edges.push_back({u, v});
        }
    }
    return g;
}

// A multigraph with m edges. Each edge is a self-loop with probability 1/20;
// otherwise, with probability about 1/10, it repeats an earlier edge (in
// either direction, so parallel edges look like the two arcs of a road in the
// DIMACS files); otherwise it joins two random distinct vertices.
// Requires n >= 1 if m > 0; for n == 1 every edge is a self-loop.
inline input_graph random_multigraph(rng& r, vertex n, std::uint32_t m) {
    assert(n >= 1 || m == 0);
    input_graph g{n, {}};
    g.edges.reserve(m);
    while (g.edges.size() < m) {
        if (n == 1 || r.chance(1, 20)) {
            const auto v = static_cast<vertex>(r.below(n));
            g.edges.push_back({v, v});
        } else if (!g.edges.empty() && r.chance(1, 10)) {
            edge e = g.edges[r.below(g.edges.size())];  // a copy: push_back may reallocate
            if (r.chance(1, 2)) {
                std::swap(e.u, e.v);
            }
            g.edges.push_back(e);
        } else {
            const auto u = static_cast<vertex>(r.below(n));
            auto v = static_cast<vertex>(r.below(n));
            while (v == u) {
                v = static_cast<vertex>(r.below(n));
            }
            g.edges.push_back({u, v});
        }
    }
    return g;
}

// A random tree with n vertices and n - 1 edges: vertex i is attached to a
// random earlier vertex, then vertex labels, edge order and edge orientation
// are shuffled. Not uniform over all trees, but every tree can occur.
inline input_graph random_tree(rng& r, vertex n) {
    std::vector<vertex> label(n);
    std::iota(label.begin(), label.end(), vertex{0});
    r.shuffle(label);
    input_graph g{n, {}};
    for (vertex i = 1; i < n; ++i) {
        edge e{label[i], label[r.below(i)]};
        if (r.chance(1, 2)) {
            std::swap(e.u, e.v);
        }
        g.edges.push_back(e);
    }
    r.shuffle(g.edges);
    return g;
}

// Default inputs for differential tests: up to 40 vertices and up to 2n edges.
// Graphs this sparse have bridges, cut vertices and cycles at the same time.
inline input_graph small_random_multigraph(rng& r) {
    const auto n = static_cast<vertex>(r.below(41));
    const auto m = static_cast<std::uint32_t>(n == 0 ? 0 : r.below(2 * std::uint64_t{n} + 1));
    return random_multigraph(r, n, m);
}

inline input_graph small_random_simple_graph(rng& r) {
    const auto n = static_cast<vertex>(r.below(41));
    const std::uint64_t max_m = std::min(2 * std::uint64_t{n}, std::uint64_t{n} * (n - 1) / 2);
    const auto m = static_cast<std::uint32_t>(n == 0 ? 0 : r.below(max_m + 1));
    return random_simple_graph(r, n, m);
}

}  // namespace elemgraph::test
