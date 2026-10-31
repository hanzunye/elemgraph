# elemgraph — Design

> Status: draft for v0.1. This document describes *why* the library looks the way it does.
> Measured results live in `bench/results/` and are summarized in the README.

## 1. Purpose

elemgraph is a small, header-only C++20 library of **elementary graph algorithms** on **static graphs**.
Its main purpose is experimental: to measure how the choice of graph representation and memory layout
affects the running time of basic algorithms, and to make every measurement reproducible.

### Research questions

| ID | Question |
|----|----------|
| Q1 | How much does a contiguous CSR layout gain over a vector-of-vectors adjacency list for BFS, DFS and connected components, and how does this depend on the graph class (grid, sparse random, power-law, road network)? |
| Q2 | Does the vertex index width (32 vs. 64 bit) matter in practice, i.e. is the workload memory-bandwidth bound? |
| Q3 | Does explicit alignment of hot arrays (cache-line alignment, 64 B vs. 128 B) measurably change performance compared to the default allocator? |
| Q4 | For bridges, cut vertices and 2-vertex/2-edge-connectivity: how does the chain-decomposition approach [Schmidt 2013] compare in practice to classic lowpoint-based DFS [Hopcroft & Tarjan 1973; Tarjan 1974]? |
| Q5 | How much does vertex reordering (e.g. BFS order, reverse Cuthill–McKee) reduce cache misses and running time? |
| Q6 | What does certification cost in practice, i.e. how expensive are producing and checking a certificate compared to the plain test? |

Negative results (an optimization that does not help) are documented, not dropped.
The roadmap (§9) lists the version in which each question is addressed.

## 2. Non-goals (v0.x)

- Not a replacement for Boost.Graph, OGDF, LEMON or NetworKit.
- No dynamic graphs: a graph is built once and then only read.
- Single-threaded first. Parallel variants only after the sequential baseline is understood.
- No graph drawing, no general-purpose I/O beyond plain edge lists and the DIMACS format.

## 3. Design principles

1. **Measure, don't guess.** Every optimization comes with a benchmark and a short note.
2. **Correctness before speed.** Each algorithm has at least two independent implementations or a
   brute-force reference, and they are cross-checked on many random graphs. Where a certificate exists,
   the algorithm returns it and a small, independent checker verifies it.
3. **Zero-cost genericity.** Algorithms are generic over the representation via C++20 concepts.
   No virtual dispatch and no type erasure in hot loops.
4. **Pay only for what you use.** Optional data (e.g. edge ids) is enabled by template parameters;
   algorithms that don't need it don't pay for it in memory or bandwidth.
5. **Reproducibility.** The toolchain is pinned with a Nix flake; benchmark results are stored together
   with machine and compiler information.

## 4. Architecture

### 4.1 Graph concepts

```cpp
template <class G>
concept StaticGraph = requires(const G& g, typename G::vertex_type v) {
    requires std::unsigned_integral<typename G::vertex_type>;
    { g.num_vertices() } -> std::convertible_to<std::size_t>;
    { g.num_edges() }    -> std::convertible_to<std::size_t>;
    { g.neighbors(v) }   -> std::ranges::forward_range;
};

// Refinement: every half-edge also knows the id of its undirected edge.
// Needed to handle parallel edges correctly in bridge / 2-connectivity algorithms.
template <class G>
concept EdgeIndexedGraph = StaticGraph<G> && requires(const G& g, typename G::vertex_type v) {
    { g.incident_edges(v) } -> std::ranges::forward_range;   // (target, edge_id) pairs
};
```

Algorithms state the weakest concept they need. A trait `has_contiguous_neighbors<G>`
(true if `neighbors(v)` is a `std::ranges::contiguous_range`) lets an algorithm pick a tighter,
index-based inner loop via `if constexpr`, so the data structure adapts to the algorithm being called.

### 4.2 Representations

| Type | Layout | Role |
|------|--------|------|
| `csr_graph<V, E, EdgeIds, Alloc>` | `offsets[n+1]` + `targets[2m]` (+ optional `edge_ids[2m]`) in contiguous arrays | main representation |
| `adjacency_list<V>` | `std::vector<std::vector<V>>` | baseline for comparison |

Template parameters:

- `V`: vertex index type (`std::uint32_t` default, `std::uint64_t` for very large graphs); checked with `static_assert`.
- `E`: offset type (independent of `V`, because `2m` can exceed `2^32` even when `n` does not).
- `EdgeIds`: `bool`; when `false` the member is removed via `std::conditional_t` / `[[no_unique_address]]`.
- `Alloc`: allocator for all arrays (see 4.3).

The parameters are introduced in stages (§9): v0.1 has `V` and `E`, always stores edge ids and uses
`std::allocator`; `EdgeIds` and `Alloc` follow in v0.2.

Construction goes through an `edge_list<V>`: the `csr_graph` constructor fills the CSR arrays with a
counting sort in `O(n + m)`. Parallel edges are kept (they matter for bridges) and self-loops are dropped.
An edge keeps its position in the edge list as its id, so results such as bridges refer to the input edges.

### 4.3 Memory alignment

- `aligned_allocator<T, Align>` built on `::operator new(size, std::align_val_t{Align})`.
- The alignment is a **template parameter**, not hard-coded, because the cache-line size is
  platform-dependent (64 B on typical x86-64; 128 B is reported on Apple Silicon).
- Per-vertex work arrays inside algorithms (visited, distance, parent, component id) use the same allocator.
- Experiment Q3 compares `Align = alignof(T)` vs. 64 vs. 128. System allocators often return large blocks
  that are already aligned far beyond `alignof(T)`, so Q3 also places arrays at deliberate offsets
  (8–64 B past a cache-line boundary); otherwise "default vs. aligned" may compare identical layouts.
- Related layout experiment: visited flags as a byte array vs. a packed 64-bit-word bitset
  (`std::vector<bool>` is deliberately avoided because its proxy references block optimizations).

### 4.4 Algorithms (v0.1–v0.2)

| Problem | Implementations | Time | Verified against |
|---------|-----------------|------|------------------|
| BFS (distances, parents) | flat-array queue (each vertex is enqueued at most once, so a size-`n` array with head/tail suffices) vs. `std::queue` | O(n+m) | each other |
| DFS (preorder, parents) | iterative, explicit stack of `(vertex, cursor)`; with CSR the cursor is just an index into `targets` | O(n+m) | recursive reference on small graphs |
| Connected components | BFS-based vs. union-find (union by size + path halving) | O(n+m) / O((n+m)·α(n)) | identical partition up to relabeling |
| Bridges, cut vertices, 2-edge-/2-vertex-connectivity | lowpoint-based DFS vs. chain decomposition | O(n+m) | each other + brute force on small graphs + certificate checker |

**Chain decomposition** [Schmidt 2013], in short: run a DFS, orient tree edges towards the root and
back edges away from it. Visit vertices in increasing DFS index; for every back edge starting at the
current vertex `v`, mark `v` and traverse the back edge and then tree edges upwards, marking vertices,
until an already marked vertex is reached. Each traversal is a *chain*; `C_1` is always a cycle.
For a simple connected graph the paper shows:

- An edge is a bridge iff it lies in no chain. So the graph is 2-edge-connected iff the chains partition `E`.
- If `δ(G) ≥ 2`, a vertex is a cut vertex iff it is incident to a bridge or is the first vertex of a cycle in `C \ C_1`.
- A 2-edge-connected graph is 2-vertex-connected iff `C_1` is the only cycle.

The paper assumes simple, connected graphs. elemgraph applies the decomposition per connected
component, allows vertices of degree 1 (an endpoint of a bridge is a cut vertex iff its degree is ≥ 2),
and keeps parallel edges, distinguished by edge id, so that a parallel copy of a tree edge becomes a back edge.
These extensions are not covered by the paper's proofs. They are argued in a short note
(`docs/chain-decomposition.md`) and checked by differential tests against the brute-force reference.
Both the chain decomposition and the lowpoint algorithm run on the same iterative DFS core.

**Certificates.** If G is 2-edge-connected, the chains form an ear decomposition of G [Robbins 1939];
if in addition `C_1` is the only cycle, they form an open ear decomposition, which certifies
2-vertex-connectivity [Whitney 1932]. Negative answers are certified by a bridge, a cut vertex, or,
for a disconnected graph, a vertex set without outgoing edges. elemgraph returns these certificates,
and a separate checker verifies them in `O(n + m)` without trusting the algorithm, in the sense of
certifying algorithms [McConnell et al. 2011]. Because the checker is linear, it also validates the
results on the large benchmark graphs, where the brute-force references below are far too slow.

**Brute-force references** (small graphs only): an edge is a bridge iff removing it increases the
number of components; a vertex is a cut vertex iff removing it does. These are `O(m·(n+m))` and `O(n·(n+m))`,
fine for `n ≤ 200` in tests.

Algorithms of later versions (certifying 3-edge-connectivity, shortest paths, parallel BFS and components,
directed graphs, minimum spanning trees) are listed in §9 and get their own section here once they are designed.

### 4.5 Workspaces

Algorithms accept an optional, reusable `workspace` object that owns the per-vertex arrays.
This separates allocation cost from algorithm cost in benchmarks and avoids repeated allocations
when an algorithm is called many times on the same graph.

## 5. Testing

- **Framework:** GoogleTest.
- **Hand-written cases:** empty graph, single vertex, self-loops, parallel edges, path, cycle, star,
  complete graph, trees (every edge is a bridge), disconnected graphs, two cycles sharing one vertex.
- **Differential testing:** thousands of seeded random graphs per run, multigraphs included; all
  implementations of the same problem must agree, and must agree with the brute-force reference on small graphs.
- **Certificate checking:** connectivity results on large random graphs are verified by the linear-time
  checker. Mutation tests corrupt valid certificates and require the checker to reject them.
- **Sanitizers:** separate ASan + UBSan build in CI (Linux).
- **Compilers:** GCC and Clang on Linux, Clang on macOS; `-Wall -Wextra -Wpedantic -Werror`.

## 6. Benchmark methodology

- **Tool:** Google Benchmark. Each benchmark is parameterized by graph class, size, representation,
  index type and alignment.
- **Graph classes:**
  - 2D grid: regular structure, high locality.
  - Erdős–Rényi `G(n, m)`: uniform random access.
  - R-MAT [Chakrabarti et al. 2004]: power-law degrees.
  - Real-world: a road network from the 9th DIMACS Implementation Challenge and one SNAP graph.
- **Input canonicalization:** benchmark inputs are simple undirected graphs. Every edge is stored as
  `(min(u,v), max(u,v))`; duplicates and self-loops are removed. This matters for bridges: the DIMACS road
  networks list every road as two opposite arcs, and R-MAT generates many duplicate edges, so a naive reader
  would turn every bridge into a pair of parallel edges. Multigraphs are used only in tests.
- **Determinism:** generators draw bounded integers and shuffles from `std::mt19937_64` with their own
  code, because `std::uniform_int_distribution` and `std::shuffle` give different results in libstdc++
  and libc++. So a seed gives the same graph on the M2 and on the x86 laptop, and in tests.
- **Sizes:** chosen to cross cache boundaries (fits in L2 → fits in LLC → DRAM-bound), roughly 10^4 to 10^7 vertices.
- **Machines:**
  - Apple M2 (macOS; 4 performance + 4 efficiency cores; 16 GB). macOS offers neither core pinning nor a
    fixed CPU frequency. Benchmarks run at the highest QoS class so that they stay on the performance cores,
    and this limitation is stated with the results.
  - An x86-64 laptop running Linux on bare metal: pinned core, fixed frequency, turbo disabled, hardware
    counters via `perf stat`.
  - CI runners are virtual machines. They run every benchmark once on tiny inputs as a smoke test;
    CI timings are never reported.
- **Metrics:**
  - Median wall time with spread over ≥ 10 repetitions.
  - Edges per second.
  - Memory footprint of the representation.
  - Hardware counters via `perf stat` (cache misses, LLC misses, branch misses; Linux only).
- **Protocol:**
  - Release build.
  - Generation and construction are timed separately from the algorithm.
  - Workspaces are preallocated, with warm-up runs before measuring.
  - Pinned core and fixed CPU frequency where the platform allows it.
- **Recording:** results stored as JSON in `bench/results/<machine>/<date>.json`, together with CPU model,
  cache sizes and compiler version; plots generated by `bench/plot.py`.

## 7. Build and reproducibility

- CMake (≥ 3.25) + Ninja. Header-only `INTERFACE` target `elemgraph::elemgraph`.
- Nix flake outputs:
  - `devShells.default`: gcc, clang, cmake, ninja, gtest, google-benchmark, python3 (pandas, matplotlib), and perf on Linux.
  - `packages.default`: installs headers and the CMake package config.
  - `checks`: on Linux, build + tests with GCC and Clang plus an ASan/UBSan build; on macOS, Clang only
    (there nixpkgs builds GoogleTest and Google Benchmark against libc++, which a GCC/libstdc++ build cannot
    link against). `nix flake check` runs all checks of the current platform.
- `flake.lock` pins nixpkgs, so every machine gets the same compiler and library versions.
- CI: GitHub Actions on Linux and macOS, running `nix flake check`.

## 8. Repository layout

```
elemgraph/
├── flake.nix, flake.lock
├── CMakeLists.txt
├── include/elemgraph/
│   ├── concepts.hpp
│   ├── aligned_allocator.hpp
│   ├── edge_list.hpp
│   ├── csr_graph.hpp, adjacency_list.hpp
│   └── algorithms/
│       ├── bfs.hpp, dfs.hpp
│       ├── components.hpp, union_find.hpp
│       ├── connectivity.hpp        # lowpoint + chain decomposition
│       └── certificates.hpp        # certificate checkers
├── tests/
├── bench/
│   ├── generators/                 # grid, G(n,m), R-MAT, DIMACS reader (also used by tests)
│   ├── *.cpp                       # Google Benchmark suites
│   ├── fetch-data.sh               # downloads DIMACS / SNAP inputs into data/
│   ├── data/                       # downloaded inputs, not in git
│   ├── results/
│   └── plot.py
├── docs/
│   ├── DESIGN.md
│   └── chain-decomposition.md      # extensions beyond the paper's assumptions
├── .github/workflows/ci.yml
├── LICENSE                         # MIT
└── README.md
```

## 9. Roadmap

| Version | Content |
|---------|---------|
| v0.1 | CSR + adjacency list, BFS / DFS / connected components, bridges / cut vertices / 2-connectivity with both approaches, tests, Nix flake, CI, first benchmarks (Q1, Q2, Q4) |
| v0.2 | Certificates and checkers (Q6), the `EdgeIds` and `Alloc` template parameters, alignment and bitset experiments (Q3), 2-edge-connected and biconnected components, perf counters on bare-metal Linux, a SNAP graph |
| v0.3 | Certifying 3-edge-connectivity [Mehlhorn, Neumann, Schmidt 2017], built on the chain decomposition |
| v0.4 | Vertex reordering (Q5), weighted CSR and Dijkstra heap variants, instruction-count regression benchmarks in CI (Cachegrind) |
| v0.5 | Scalability: direction-optimizing BFS, parallel BFS and connected components |
| v0.6 | Directed graphs (strongly connected components, topological sort), minimum spanning trees |

## 10. License

MIT. See `LICENSE`.

## References

- H. Whitney. *Non-separable and planar graphs.* Transactions of the American Mathematical Society 34(2):339–362, 1932.
- H. E. Robbins. *A theorem on graphs, with an application to a problem of traffic control.* The American Mathematical Monthly 46(5):281–283, 1939.
- J. Hopcroft, R. Tarjan. *Algorithm 447: Efficient algorithms for graph manipulation.* Communications of the ACM 16(6), 1973.
- R. E. Tarjan. *A note on finding the bridges of a graph.* Information Processing Letters 2(6), 1974.
- R. E. Tarjan. *Efficiency of a good but not linear set union algorithm.* Journal of the ACM 22(2), 1975.
- R. M. McConnell, K. Mehlhorn, S. Näher, P. Schweitzer. *Certifying algorithms.* Computer Science Review 5(2):119–161, 2011.
- J. M. Schmidt. *A simple test on 2-vertex- and 2-edge-connectivity.* Information Processing Letters 113(7):241–244, 2013. doi:10.1016/j.ipl.2013.01.016. arXiv:1209.0700.
- K. Mehlhorn, A. Neumann, J. M. Schmidt. *Certifying 3-edge-connectivity.* Algorithmica 77:309–335, 2017. doi:10.1007/s00453-015-0075-x. arXiv:1211.6553.
- D. Chakrabarti, Y. Zhan, C. Faloutsos. *R-MAT: A recursive model for graph mining.* SIAM SDM, 2004.
- 9th DIMACS Implementation Challenge: Shortest Paths. https://www.diag.uniroma1.it/challenge9/
- SNAP: Stanford Large Network Dataset Collection. https://snap.stanford.edu/data/
