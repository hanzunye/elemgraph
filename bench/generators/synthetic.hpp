// Synthetic benchmark inputs (DESIGN.md §6). Every generator returns a
// canonical simple graph (see canonicalize()), and the same arguments give the
// same graph on every platform.
#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

#include "input_graph.hpp"
#include "rng.hpp"

namespace elemgraph::gen {

// rows x cols grid. Vertex (r, c) gets id r * cols + c, so the neighbours of a
// vertex are close to it in memory: the regular, high-locality graph class.
// The edges are generated in canonical order.
inline input_graph grid(vertex rows, vertex cols) {
    assert(std::uint64_t{rows} * cols < std::numeric_limits<vertex>::max());
    input_graph g{rows * cols, {}};
    g.edges.reserve(2 * std::size_t{rows} * cols);
    for (vertex r = 0; r < rows; ++r) {
        for (vertex c = 0; c < cols; ++c) {
            const vertex v = r * cols + c;
            if (c + 1 < cols) {
                g.edges.push_back({v, v + 1});
            }
            if (r + 1 < rows) {
                g.edges.push_back({v, v + cols});
            }
        }
    }
    return g;
}

// Erdős–Rényi G(n, m): m distinct edges chosen uniformly at random among all
// n (n - 1) / 2 vertex pairs. Self-loops and repeats are dropped and drawn
// again, which is cheap for sparse graphs. Requires m <= n (n - 1) / 2.
inline input_graph gnm(vertex n, std::uint32_t m, std::uint64_t seed) {
    assert(2 * std::uint64_t{m} <= std::uint64_t{n} * (n - 1));
    rng r(seed);
    input_graph g{n, {}};
    g.edges.reserve(m);
    while (g.edges.size() < m) {
        for (std::size_t missing = m - g.edges.size(); missing > 0; --missing) {
            const auto u = static_cast<vertex>(r.below(n));
            const auto v = static_cast<vertex>(r.below(n));
            g.edges.push_back({u, v});
        }
        canonicalize(g);
    }
    return g;
}

// R-MAT [Chakrabarti, Zhan, Faloutsos 2004] with the Graph500 parameters
// a = 0.57, b = c = 0.19, d = 0.05: power-law degrees. The graph has
// n = 2^scale vertices; edge_factor * n edges are drawn, each by picking one
// quadrant of the adjacency matrix per bit of the vertex ids. Self-loops and
// repeats are then removed, so the result has fewer edges than were drawn.
// As in Graph500, vertex ids are shuffled at the end; otherwise a vertex's
// degree would follow from the number of zero bits in its id.
inline input_graph rmat(unsigned scale, std::uint32_t edge_factor, std::uint64_t seed) {
    assert(scale < 32);
    const vertex n = vertex{1} << scale;
    const std::uint64_t draws = std::uint64_t{edge_factor} * n;
    assert(draws <= std::numeric_limits<std::uint32_t>::max());
    rng r(seed);
    input_graph g{n, {}};
    g.edges.reserve(draws);
    for (std::uint64_t i = 0; i < draws; ++i) {
        vertex u = 0;
        vertex v = 0;
        for (unsigned bit = 0; bit < scale; ++bit) {
            // Quadrants a, b, c, d in percent: [0, 57), [57, 76), [76, 95), [95, 100).
            // c and d are the lower half (u gets a 1), b and d the right half (v gets a 1).
            const std::uint64_t x = r.below(100);
            u = (u << 1) | static_cast<vertex>(x >= 76);
            v = (v << 1) | static_cast<vertex>((x >= 57 && x < 76) || x >= 95);
        }
        g.edges.push_back({u, v});
    }
    std::vector<vertex> new_id(n);
    std::iota(new_id.begin(), new_id.end(), vertex{0});
    r.shuffle(new_id);
    for (edge& e : g.edges) {
        e = {new_id[e.u], new_id[e.v]};
    }
    canonicalize(g);
    return g;
}

}  // namespace elemgraph::gen
