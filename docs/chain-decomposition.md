# The chain decomposition on general graphs

`chain_connectivity()` in `include/elemgraph/algorithms/connectivity.hpp` finds the bridges and cut
vertices of a graph with the chain decomposition of [Schmidt 2013]. The paper proves its
characterizations for simple connected graphs, and the one for cut vertices only for graphs of
minimum degree 2. elemgraph accepts any undirected graph: it may have parallel edges, several
connected components, and vertices of degree 0 or 1.

This note states the rules that the implementation uses and proves them in this general setting.
The proofs use only standard properties of depth-first search and follow the ideas of the paper.
For a simple connected graph with minimum degree 2, the rules are exactly those of the paper.

## 1. Setting

G = (V, E) is a finite undirected multigraph with n vertices and m edges. Edges are told apart by
their ids, so several edges may have the same two ends. deg(v) is the number of edges at v.

Self-loops are removed first (`csr_graph` drops them). This changes no answer. A self-loop is never
a bridge, and the components of G − e and of G − v are the same with or without self-loops.

An edge e is a **bridge** if G − e has more connected components than G. A vertex v is a
**cut vertex** if G − v has more connected components than G. A vertex of degree 0 or 1 is never
a cut vertex, because no path between two other vertices passes through it.

## 2. Depth-first search

`dfs()` searches all components and numbers the vertices in preorder. We write u < v if u has
the smaller DFS number. Each component is searched by one DFS tree, and the vertices of a tree
form an interval of the preorder that starts with its root.

For a vertex x, T(x) is the set of descendants of x, x included. If x is not a root, p(x) is its
parent, and t(x) is the **tree edge** through which the search reached x. If x and p(x) are joined
by parallel edges, exactly one of them is t(x). The others are non-tree edges.

**(D1) No cross edges.** Every non-tree edge joins a vertex to one of its proper descendants.
Parallel edges do not change the usual argument. Let u be the end of the edge that the search
reached first. The search leaves u only after it has scanned this edge, so it reaches the other
end while u is still on the stack. That end is therefore a descendant of u.

We orient every non-tree edge from its ancestor end to its descendant end and call it a
**back edge** (v, w), so v < w. A parallel copy of t(x) is the back edge (p(x), x). A back edge
(v, w) **covers** a vertex x if w ∈ T(x) and v is a proper ancestor of x, that is, if t(x) lies on
the tree path from w to v.

**(D2) Bridges.** A back edge (v, w) is never a bridge, because it forms a cycle with the tree path
from w to v. A tree edge t(x) is a bridge iff no back edge covers x.

*Proof.* In G − t(x), the vertices of T(x) are still connected by tree edges, and so are the other
vertices of x's tree. So t(x) is a bridge iff no edge other than t(x) leaves T(x). Such an edge is
a non-tree edge. By (D1), its end outside T(x) is an ancestor of its end inside, since every
descendant of a vertex in T(x) is in T(x) as well. That makes the outside end a proper ancestor of
x. So such an edge is exactly a back edge that covers x. □

**(D3) Cut vertices** [Hopcroft & Tarjan 1973]. A root is a cut vertex iff it has at least two
children. A vertex v that is not a root is a cut vertex iff it has a child c such that no back
edge (u, w) has w ∈ T(c) and u a proper ancestor of v.

*Proof.* Removing v from its tree leaves several connected pieces. These are the subtrees T(c) of
the children of v and, if v is not a root, the part of the tree above v. By (D1), no edge joins
two different subtrees T(c). An edge from T(c) to the part above is a back edge (u, w) with
w ∈ T(c) and u a proper ancestor of v.

If v is a root, the pieces stay separate in G − v, so G − v has more components than G iff there
are at least two pieces. If v is not a root, G − v has more components than G iff some T(c) has no
edge to the part above. □

These are the facts on which the lowpoint algorithm (`lowpoint_connectivity()`) is based. None of
them needs a simple or connected graph, or a minimum degree.

## 3. The chain decomposition

The chains are built as in the paper, in every tree of the DFS forest:

> Go through the vertices v in increasing DFS number. For each back edge (v, w), in any order,
> mark v. Then follow (v, w) and then tree edges upwards from w, marking every vertex passed,
> and stop at the first vertex that was already marked. The edges followed form a **chain** whose
> first vertex is v.

Since v is marked and is an ancestor of w, the walk stops at v at the latest. So a chain consists
of a back edge (v, w) and the tree path from w up to some vertex x on the path from w to v. It is
a **cycle** if x = v and a **path** otherwise. Three properties follow directly:

- **(C1)** Each back edge lies in exactly one chain, and each tree edge in at most one. A walk
  follows t(y) only from an unmarked vertex y, and it marks y.
- **(C2)** A chain with first vertex v stays in T(v). So chains in different trees do not interact.
- **(C3)** The first chain of each tree is a cycle, because when it is built, its first vertex is
  the only marked vertex in the tree. We call this chain the **C₁ of the tree**.

Compared with the paper, the procedure differs in one point only: a parallel copy of a tree edge
t(x) counts as a back edge (p(x), x). If x is still unmarked, its chain p(x), x, p(x) is a cycle of
length 2.

**Lemma 1.** Let x be a vertex that is not a root, and let e = (v, w) be the first back edge
covering x that the procedure processes. Then the chain of e contains t(x).

*Proof.* The walk of e starts in T(x) at w and climbs towards x. Assume it stops at a vertex
y ∈ T(x), which means y was marked before e was processed. Chains are built in increasing DFS
number of their first vertex, and v < x ≤ y, so y has not started a chain yet. Hence an earlier
chain climbed from y to p(y). It belongs to a back edge (v', w') with w' ∈ T(y) and v' a proper
ancestor of y. It was processed before e, so v' ≤ v < x, and therefore v' ∉ T(x). So v' is a
proper ancestor of x. Then (v', w') covers x and was processed before e, a contradiction. So the
walk leaves T(x), which it can do only through t(x). □

The order of the back edges at a vertex is never used. Any order gives a valid chain
decomposition.

**Theorem 1 (bridges).** An edge is a bridge iff it lies in no chain. So the chains partition the
edges that are not bridges.

*Proof.* Back edges lie in a chain (C1) and are not bridges (D2). Let t(x) be a tree edge. If a
chain contains t(x), it climbs from a vertex w ∈ T(x) past x. Its back edge (v, w) covers x, so
t(x) is not a bridge by (D2). If t(x) is not a bridge, some back edge covers x (D2). By Lemma 1,
a chain contains t(x). The partition then follows from (C1). □

For a connected graph, this is the paper's test for 2-edge-connectivity: G has no bridge iff its
chains partition E.

**Theorem 2 (cut vertices).** A vertex v is a cut vertex iff

- **(a)** v is an end of a bridge and deg(v) ≥ 2, or
- **(b)** v is the first vertex of a chain that is a cycle and is not the C₁ of v's tree.

For a simple connected graph with minimum degree 2, every vertex has degree at least 2 and there
is only one tree. So (a) and (b) say the same as the paper: v is incident to a bridge, or v is
the first vertex of a cycle of the decomposition other than C₁.

*Proof that (a) makes v a cut vertex.* Let {v, z} be a bridge, and let {v, u} be another edge at v.
Then u ≠ z, because an edge parallel to the bridge would still join v and z after the bridge is
removed. When the bridge is removed, u stays joined to v but z does not. So every path from u
to z uses the bridge, and with it the vertex v. In G − v, u and z therefore lie in different
components, although they lie in the same component of G.

*Proof that (b) makes v a cut vertex.* Let C be a cycle with first vertex v that is not C₁. Let c
be the child of v whose tree edge t(c) is the last edge of C.

- If v is a root, the C₁ of its tree also starts at v, because no vertex of the tree comes before
  v. C₁ is a cycle, so it ends with the tree edge t(c₁) of some child c₁. By (C1), c₁ ≠ c. So v has
  two children and is a cut vertex by (D3).
- If v is not a root, assume some back edge (u, w) had w ∈ T(c) and u a proper ancestor of v. This
  back edge covers c and is processed before all back edges at v. So the first back edge covering
  c starts above v as well, and by Lemma 1 its chain contains t(c). That chain is not C, which
  contradicts (C1). So no such back edge exists, and v is a cut vertex by (D3).

*Proof that every cut vertex satisfies (a) or (b).* Let v be a cut vertex, and choose a child c by
(D3). If v is a root, c is any child, and v has a second child c'.

If t(c) is a bridge, (a) holds. v has the edge t(c) and a second edge, t(v) or t(c').

Otherwise some back edge covers c (D2). It starts at a proper ancestor of c. If v is a root, that
ancestor can only be v. If v is not a root, the choice of c rules out the proper ancestors of v,
so again it is v. By Lemma 1, the first back edge covering c, which starts at v, has a chain that
contains t(c). This chain is a cycle with first vertex v; call it C. If C is not C₁, (b) holds.
If C is C₁:

- If v is not a root, no vertex before v in its tree starts a chain. So no back edge starts at a
  proper ancestor of v. Then no back edge covers v, t(v) is a bridge by (D2), and (a) holds, since
  v also has the edge t(c).
- If v is a root and t(c') is a bridge, (a) holds. Otherwise the argument above, applied to c',
  gives a cycle C' with first vertex v that contains t(c'). Every tree edge of C is t(y) for some
  y ∈ T(c), so C does not contain t(c'), and C' ≠ C. Hence C' is not C₁, and (b) holds. □

## 4. Why each extension is needed

Each of the following graphs is in `tests/algorithms/chains_test.cpp`.

- **Several components.** Take two disjoint triangles. If C₁ meant the first chain of the whole
  graph instead of each tree, the root of the second triangle would start a cycle other than C₁
  and would wrongly be reported as a cut vertex.
- **Vertices of degree 1.** Take the star with edges 0 – 1, 1 – 2 and 1 – 3. All three edges are
  bridges, and every vertex is incident to one. Only vertex 1 is a cut vertex, and only the
  condition deg(v) ≥ 2 in (a) tells it apart from the leaves.
- **Parallel edges.** Take the edge 0 – 1 and two parallel edges between 1 and 2. The search uses
  one of the parallel edges as the tree edge t(2). The other one is the back edge (1, 2), and its
  chain 1, 2, 1 is a cycle of length 2. So only 0 – 1 is a bridge, and 1 is a cut vertex by (a).
  Skipping every edge from a vertex to its child, instead of only the tree edge, would report
  t(2) as a bridge, and 2 as a cut vertex.

## 5. From the rules to the code

`detail::chain_connectivity()` in `connectivity.hpp` follows Section 3:

- `dfs()` stores the preorder, the DFS numbers and the parents, but not the tree edges. t(x) is
  the first edge to p(x) in the list of x, and also the first edge to x in the list of p(x).
  `dfs.hpp` explains why: both lists are ordered by edge id.
- At v, an edge to w with `number[w] > number[v]` goes to a descendant (D1). The first such edge
  to each child w is t(w), which the code skips with `tree_edge_seen[w]`. Every other edge to a
  descendant is a back edge (v, w), parallel copies of t(w) included.
- `state[x]` is the mark: `chain_start` for a first vertex, and `climbed` for a vertex from which
  a walk followed t(x). So t(x) lies in a chain iff `state[x] == climbed`. By Theorem 1, the
  bridges are the t(x) with `state[x] != climbed`.
- A chain is a cycle iff its walk stops at v. `tree_has_chain` is reset at every root, so the
  first chain of each tree is that tree's C₁, as in Theorem 2 (b). Rule (a) is applied to both
  ends of every bridge.

The running time is O(n + m). Every edge is scanned a constant number of times, and every vertex
is climbed from at most once. `tree_edge(x)` scans the list of x. It is called once for each bridge
and, if the chains are recorded, once for each climbed vertex.

## 6. How this is checked

- `tests/algorithms/chains_random_test.cpp` runs `chain_connectivity()` on 1000 random multigraphs
  (up to 40 vertices and 2n edges, self-loops and parallel edges included), 1000 random simple
  graphs and 200 random trees. In every case it gives the same bridges and cut vertices as
  `lowpoint_connectivity()`. They also match brute-force references, which check the definitions of
  Section 1 by removing each edge and each vertex in turn.
- On 1000 further multigraphs, the test checks Theorem 1, (C1) and (C3) on the recorded chains.
  Every edge that is neither a bridge nor a self-loop lies in exactly one chain. Chains appear in
  increasing DFS number of their first vertex. The first chain of each tree is a cycle.
- Three deliberate mistakes each make these tests fail: C₁ taken once for the whole graph, the
  condition deg(v) ≥ 2 left out of (a), and the tree edges to children taken as back edges.

## References

- J. M. Schmidt. *A simple test on 2-vertex- and 2-edge-connectivity.* Information Processing
  Letters 113(7):241–244, 2013. doi:10.1016/j.ipl.2013.01.016. arXiv:1209.0700.
- J. Hopcroft, R. Tarjan. *Algorithm 447: Efficient algorithms for graph manipulation.*
  Communications of the ACM 16(6), 1973.
