# Changelog

## v0.1.0 (unreleased)

This is the first release. It contains two graph representations and the elementary algorithms,
two ways to find bridges and cut vertices, tests against brute-force references, and the first
experiments (Q1 and Q4 in [`docs/DESIGN.md`](docs/DESIGN.md)) on an Apple M2.

### Library

- `edge_list`: the input. An edge's position in the list is its id.
- `csr_graph`: a compressed sparse row graph, built by counting sort. `adjacency_list`: one
  `std::vector` per vertex. Both keep parallel edges and drop self-loops, and both list the
  neighbors of each vertex in order of edge id.
- The concepts `StaticGraph` and `EdgeIndexedGraph`, so that each algorithm runs on both
  representations.
- BFS, with a flat-array queue and with `std::queue`.
- Iterative DFS over all components.
- Connected components with BFS and with union-find (union by size, path halving).
- Bridges and cut vertices with the lowpoint algorithm and with the chain decomposition
  [Schmidt 2013]. The second can also return the chains. Both take the same DFS result, so they
  can be timed apart from the search.
- Every algorithm has a version that reuses its memory (a workspace) across calls.

### Correctness

- Differential tests against brute-force references on random multigraphs, simple graphs and
  trees, with parallel edges, self-loops and several components.
- [`docs/chain-decomposition.md`](docs/chain-decomposition.md) proves the chain decomposition's
  rules for graphs with parallel edges, several components and vertices of degree 0 or 1, which
  the paper does not cover.
- CI with `nix flake check`: GCC, Clang, and AddressSanitizer with UndefinedBehaviorSanitizer on
  Linux; Clang on macOS; one run of every benchmark on tiny graphs.

### Benchmarks

- Generators for 2D grids, G(n,m) and R-MAT with fixed seeds, which give the same graphs on every
  platform. A reader for DIMACS road networks.
- Suites for Q1 (BFS on both representations) and Q4 (DFS, lowpoint and chain decomposition,
  timed separately), from 2^12 to 2^22 vertices.
- `bench/run.sh` runs the suites and records the machine's settings: pinned core, governor, turbo
  and hardware counters on Linux. `bench/plot.py` draws the figures and tables.

### Results on the Apple M2

- **Q4.** After the shared DFS, the lowpoint algorithm is faster in 20 of 24 cases: 1.2–1.3× on
  grids, and 1.3–1.9× on G(n,m) and R-MAT while the graph fits in the caches. On G(n,m) with 2^19
  vertices and more, the chain decomposition is 3–5 % faster. With the DFS included, the two are
  within 0.98–1.09× of each other from 2^20 vertices on.
- **Q1.** CSR is usually 3–35 % faster than the adjacency list for BFS. On G(n,m) and R-MAT with
  2^19 and 2^20 vertices it is 1.7–1.9× faster. These are the sizes at which the per-vertex data
  that BFS reads in random order fits in the 16 MB L2 cache for CSR but not for the adjacency
  list.

Details: [`bench/results/m2/2026-11-11.md`](bench/results/m2/2026-11-11.md).

### Known limitations

- All results come from one machine. The x86-64 Linux runs with hardware counters, which are to
  check the explanations above, are still to come.
- Vertex ids and edge ids are fixed to 32 bits. They become template parameters in v0.2 (Q2).
- The benchmarks use synthetic graphs only. The DIMACS reader exists, but no road network is
  downloaded yet.
