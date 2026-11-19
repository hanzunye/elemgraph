// Certifying tests for 2-edge-connectivity and 2-vertex-connectivity
// (DESIGN.md §4.4). Each test answers yes or no and returns a certificate for
// its answer, which a checker verifies in O(n + m) time from the graph alone:
//
//   answer  2-edge-connected               2-vertex-connected
//   yes     an ear decomposition           an open ear decomposition
//   no      a set of vertices that at      a set of vertices that has no edge
//           most one edge leaves           to the rest once at most one vertex
//                                          is removed
//
// A graph with too few vertices (2-edge: fewer than 2; 2-vertex: fewer than 3)
// is neither, and needs no certificate for no. The tests take the
// certificates from the chain decomposition [Schmidt 2013]: its chains are
// the ears. docs/certifying-connectivity.md proves that each certificate
// proves its answer, and that the tests always find one.
//
// The checkers use nothing but the graph and the certificate. They call no
// search and no other algorithm of the library, so a bug in the DFS or in the
// chain decomposition cannot make them accept a wrong answer
// [McConnell et al. 2011].
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "../concepts.hpp"
#include "../edge_list.hpp"
#include "connectivity.hpp"
#include "dfs.hpp"

namespace elemgraph {

// The answer to "is g 2-edge-connected?": does g have at least two vertices,
// and is g connected after removing any one edge (or none)?
struct two_edge_connectivity_certificate {
    bool two_edge_connected = false;
    // If yes: an ear decomposition of g. The first ear is a cycle. Each
    // further ear is a path or a cycle whose ends are on earlier ears and
    // whose other vertices are on none. Every vertex and every edge of g is
    // on an ear, and every edge on only one.
    chain_decomposition ears;
    // If no and g has at least two vertices: a set of vertices, neither empty
    // nor all of them, that at most one edge leaves.
    std::vector<vertex> side;
};

// The answer to "is g 2-vertex-connected?": does g have at least three
// vertices, and is g connected after removing any one vertex (or none)?
struct two_vertex_connectivity_certificate {
    bool two_vertex_connected = false;
    // If yes: an open ear decomposition of g, that is, an ear decomposition
    // in which every ear but the first is a path with two different ends.
    chain_decomposition ears;
    // If no and g has at least three vertices: the vertices of g split into
    // `side`, the `separator` (one vertex, or no_vertex for none) and the
    // rest, with `side` and the rest not empty and no edge between them.
    vertex separator = no_vertex;
    std::vector<vertex> side;
};

// Memory that the tests reuse from one call to the next.
struct certifying_workspace {
    chain_workspace chains;
    connectivity_result result;
};

namespace detail {

// Whether the DFS forest has more than one tree, i.e. g is disconnected.
inline bool has_second_tree(const dfs_result& search) {
    for (std::size_t k = 1; k < search.order.size(); ++k) {
        if (search.parent[search.order[k]] == no_vertex) {
            return true;
        }
    }
    return false;
}

// Appends the vertices of the DFS subtree of x to `out`, x first. They form
// the block of the preorder that starts at x: a later vertex belongs to it
// iff its parent does, that is, iff its parent's DFS number is at least x's.
inline void append_subtree(const dfs_result& search, vertex x, std::vector<vertex>& out) {
    out.push_back(x);
    for (std::size_t k = std::size_t{search.number[x]} + 1; k < search.order.size(); ++k) {
        const vertex y = search.order[k];
        const vertex p = search.parent[y];
        if (p == no_vertex || search.number[p] < search.number[x]) {
            break;
        }
        out.push_back(y);
    }
}

// The lower end x of the first bridge t(x) in preorder, or no_vertex if
// there is none. `state` is chain_workspace::state after chain_connectivity():
// t(x) is a bridge iff no chain climbed from x (Theorem 1 in
// docs/chain-decomposition.md).
inline vertex first_bridge_end(const dfs_result& search, const std::vector<std::uint8_t>& state) {
    for (const vertex x : search.order) {
        if (search.parent[x] != no_vertex && state[x] != climbed) {
            return x;
        }
    }
    return no_vertex;
}

}  // namespace detail

// Overwrites `certificate`. `search` must be dfs() of g.
template <EdgeIndexedGraph Graph>
void certify_two_edge_connectivity(const Graph& g, const dfs_result& search,
                                   two_edge_connectivity_certificate& certificate,
                                   certifying_workspace& workspace) {
    certificate.two_edge_connected = false;
    certificate.ears.clear();
    certificate.side.clear();
    if (g.num_vertices() < 2) {
        return;
    }
    chain_connectivity(g, search, workspace.result, workspace.chains, certificate.ears);

    if (detail::has_second_tree(search)) {
        // The first tree is a component: no edge leaves it.
        detail::append_subtree(search, search.order[0], certificate.side);
    } else if (const vertex x = detail::first_bridge_end(search, workspace.chains.state);
               x != no_vertex) {
        // Only the bridge t(x) leaves the subtree of x.
        detail::append_subtree(search, x, certificate.side);
    } else {
        certificate.two_edge_connected = true;  // the chains are the ears
        return;
    }
    certificate.ears.clear();
}

// Overwrites `certificate`. `search` must be dfs() of g.
template <EdgeIndexedGraph Graph>
void certify_two_vertex_connectivity(const Graph& g, const dfs_result& search,
                                     two_vertex_connectivity_certificate& certificate,
                                     certifying_workspace& workspace) {
    certificate.two_vertex_connected = false;
    certificate.ears.clear();
    certificate.separator = no_vertex;
    certificate.side.clear();
    if (g.num_vertices() < 3) {
        return;
    }
    chain_decomposition& chains = certificate.ears;
    chain_connectivity(g, search, workspace.result, workspace.chains, chains);

    if (detail::has_second_tree(search)) {
        // The first tree is a component: nothing separates it from the rest.
        detail::append_subtree(search, search.order[0], certificate.side);
        chains.clear();
        return;
    }
    if (const vertex x = detail::first_bridge_end(search, workspace.chains.state);
        x != no_vertex) {
        // Only the bridge t(x) leaves the subtree of x. If x has another
        // edge, x separates its descendants from the rest; otherwise x is a
        // leaf and its parent separates it from the rest, which is not empty
        // because g has at least three vertices.
        detail::append_subtree(search, x, certificate.side);
        if (g.degree(x) >= 2) {
            certificate.separator = x;
            certificate.side.erase(certificate.side.begin());
        } else {
            certificate.separator = search.parent[x];
        }
        chains.clear();
        return;
    }
    for (std::size_t i = 1; i < chains.size(); ++i) {
        if (chains.is_cycle(i)) {
            // A cycle other than C_1 that starts at v and ends with the tree
            // edge t(c): v separates the subtree of c from the rest
            // (docs/certifying-connectivity.md, Lemma 3).
            const auto vs = chains.vertices(i);
            const vertex v = vs.front();
            const vertex c = vs[vs.size() - 2];
            certificate.separator = v;
            detail::append_subtree(search, c, certificate.side);
            chains.clear();
            return;
        }
    }
    certificate.two_vertex_connected = true;  // the chains are the ears
}

// Convenience versions: run dfs() too and allocate their own memory.
template <EdgeIndexedGraph Graph>
[[nodiscard]] two_edge_connectivity_certificate certify_two_edge_connectivity(const Graph& g) {
    two_edge_connectivity_certificate certificate;
    certifying_workspace workspace;
    certify_two_edge_connectivity(g, dfs(g), certificate, workspace);
    return certificate;
}

template <EdgeIndexedGraph Graph>
[[nodiscard]] two_vertex_connectivity_certificate certify_two_vertex_connectivity(const Graph& g) {
    two_vertex_connectivity_certificate certificate;
    certifying_workspace workspace;
    certify_two_vertex_connectivity(g, dfs(g), certificate, workspace);
    return certificate;
}

// Memory that the checkers reuse from one call to the next.
struct certificate_check_workspace {
    std::vector<vertex> end_a;        // the ends of each edge, by id; no_vertex if g has no such edge
    std::vector<vertex> end_b;
    std::vector<std::uint8_t> used;   // per edge id: on an ear already
    std::vector<std::uint8_t> mark;   // per vertex
};

namespace detail {

// Whether `ears` is an ear decomposition of g (see
// two_edge_connectivity_certificate), and an open one if `open`.
template <EdgeIndexedGraph Graph>
bool is_ear_decomposition(const Graph& g, const chain_decomposition& ears, bool open,
                          certificate_check_workspace& workspace) {
    const std::size_t n = g.num_vertices();

    // The two ends of every edge, by id. Each edge is listed at both ends.
    std::size_t ids = 0;
    for (vertex v = 0; v < n; ++v) {
        for (const edge_id e : g.edge_ids(v)) {
            ids = std::max(ids, std::size_t{e} + 1);
        }
    }
    std::vector<vertex>& end_a = workspace.end_a;
    std::vector<vertex>& end_b = workspace.end_b;
    end_a.assign(ids, no_vertex);
    end_b.assign(ids, no_vertex);
    for (vertex v = 0; v < n; ++v) {
        const auto neighbors = g.neighbors(v);
        const auto edge_ids = g.edge_ids(v);
        for (std::size_t j = 0; j < neighbors.size(); ++j) {
            if (end_a[edge_ids[j]] == no_vertex) {
                end_a[edge_ids[j]] = v;
                end_b[edge_ids[j]] = neighbors[j];
            }
        }
    }
    std::vector<std::uint8_t>& used = workspace.used;
    used.assign(ids, 0);
    std::vector<std::uint8_t>& on_ear = workspace.mark;
    on_ear.assign(n, 0);
    std::size_t vertices_covered = 0;
    std::size_t edges_covered = 0;

    if (ears.size() == 0) {
        return false;
    }
    for (std::size_t i = 0; i < ears.size(); ++i) {
        const auto vs = ears.vertices(i);
        // At least one edge. Checked before edges(i) is called, which relies
        // on every earlier ear having at least one vertex.
        if (vs.size() < 2) {
            return false;
        }
        const auto es = ears.edges(i);
        const vertex first = vs.front();
        const vertex last = vs.back();
        if (first >= n || last >= n) {
            return false;
        }
        if (i == 0) {
            if (first != last) {
                return false;  // the first ear is a cycle
            }
            on_ear[first] = 1;
            ++vertices_covered;
        } else if (on_ear[first] == 0 || on_ear[last] == 0 || (open && first == last)) {
            return false;
        }
        // The vertices between the ends are new, and all different.
        for (std::size_t j = 1; j + 1 < vs.size(); ++j) {
            const vertex x = vs[j];
            if (x >= n || on_ear[x] != 0) {
                return false;
            }
            on_ear[x] = 1;
            ++vertices_covered;
        }
        // Each edge joins the two vertices it is listed between, and is on
        // no other ear.
        for (std::size_t j = 0; j < es.size(); ++j) {
            const edge_id e = es[j];
            if (e >= ids || end_a[e] == no_vertex || used[e] != 0) {
                return false;
            }
            const vertex a = vs[j];
            const vertex b = vs[j + 1];
            if (!((end_a[e] == a && end_b[e] == b) || (end_a[e] == b && end_b[e] == a))) {
                return false;
            }
            used[e] = 1;
            ++edges_covered;
        }
    }
    return vertices_covered == n && edges_covered == g.num_edges();
}

// Whether `side` is a set of vertices of g, neither empty nor all of them,
// that at most one edge leaves.
template <StaticGraph Graph>
bool is_small_edge_cut(const Graph& g, const std::vector<vertex>& side,
                       std::vector<std::uint8_t>& in_side) {
    const std::size_t n = g.num_vertices();
    in_side.assign(n, 0);
    if (side.empty() || side.size() >= n) {
        return false;
    }
    for (const vertex v : side) {
        if (v >= n || in_side[v] != 0) {
            return false;
        }
        in_side[v] = 1;
    }
    std::size_t leaving = 0;
    for (const vertex v : side) {
        for (const vertex w : g.neighbors(v)) {
            leaving += in_side[w] == 0 ? 1 : 0;
        }
    }
    return leaving <= 1;
}

// Whether `side`, `separator` and the remaining vertices of g split its
// vertices into three sets, with `side` and the rest not empty and no edge
// between them.
template <StaticGraph Graph>
bool is_separation(const Graph& g, vertex separator, const std::vector<vertex>& side,
                   std::vector<std::uint8_t>& part) {
    const std::size_t n = g.num_vertices();
    constexpr std::uint8_t rest = 0;
    constexpr std::uint8_t in_side = 1;
    constexpr std::uint8_t in_separator = 2;
    part.assign(n, rest);
    std::size_t taken = side.size();
    if (separator != no_vertex) {
        if (separator >= n) {
            return false;
        }
        part[separator] = in_separator;
        ++taken;
    }
    if (side.empty() || taken >= n) {
        return false;
    }
    for (const vertex v : side) {
        if (v >= n || part[v] != rest) {
            return false;
        }
        part[v] = in_side;
    }
    for (const vertex v : side) {
        for (const vertex w : g.neighbors(v)) {
            if (part[w] == rest) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace detail

// Whether `certificate` proves its answer for g.
template <EdgeIndexedGraph Graph>
[[nodiscard]] bool check_two_edge_connectivity(const Graph& g,
                                               const two_edge_connectivity_certificate& certificate,
                                               certificate_check_workspace& workspace) {
    if (certificate.two_edge_connected) {
        return g.num_vertices() >= 2 &&
               detail::is_ear_decomposition(g, certificate.ears, false, workspace);
    }
    return g.num_vertices() < 2 || detail::is_small_edge_cut(g, certificate.side, workspace.mark);
}

template <EdgeIndexedGraph Graph>
[[nodiscard]] bool check_two_vertex_connectivity(
    const Graph& g, const two_vertex_connectivity_certificate& certificate,
    certificate_check_workspace& workspace) {
    if (certificate.two_vertex_connected) {
        // An open ear decomposition alone allows two vertices joined by
        // parallel edges, which is not 2-vertex-connected.
        return g.num_vertices() >= 3 &&
               detail::is_ear_decomposition(g, certificate.ears, true, workspace);
    }
    return g.num_vertices() < 3 ||
           detail::is_separation(g, certificate.separator, certificate.side, workspace.mark);
}

// Convenience versions that allocate their own memory.
template <EdgeIndexedGraph Graph>
[[nodiscard]] bool check_two_edge_connectivity(
    const Graph& g, const two_edge_connectivity_certificate& certificate) {
    certificate_check_workspace workspace;
    return check_two_edge_connectivity(g, certificate, workspace);
}

template <EdgeIndexedGraph Graph>
[[nodiscard]] bool check_two_vertex_connectivity(
    const Graph& g, const two_vertex_connectivity_certificate& certificate) {
    certificate_check_workspace workspace;
    return check_two_vertex_connectivity(g, certificate, workspace);
}

}  // namespace elemgraph
