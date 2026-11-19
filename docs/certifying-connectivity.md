# Certifying 2-edge- and 2-vertex-connectivity

`certify_two_edge_connectivity()` and `certify_two_vertex_connectivity()` in
`include/elemgraph/algorithms/certifying.hpp` answer whether a graph is 2-edge-connected or
2-vertex-connected. Each returns a certificate for its answer. `check_two_edge_connectivity()` and
`check_two_vertex_connectivity()` verify such a certificate in O(n + m) time, using only the graph
and the certificate. This is a certifying algorithm in the sense of [McConnell et al. 2011]: a user
who trusts the short checker need not trust the search or the chain decomposition.

This note proves two things:

- **Soundness** (Section 2): every certificate that a checker accepts proves its answer. This is
  what makes the checkers worth running.
- **Completeness** (Section 3): the tests always return a certificate that the checkers accept.

Section 3 builds on [chain-decomposition.md](chain-decomposition.md) and uses its notation and
results: T(x), t(x), back edges, (D1)–(D3), (C1)–(C3), Lemma 1, and Theorems 1 and 2.

## 1. Definitions

G = (V, E) is a finite undirected multigraph with n vertices. Edges are told apart by their ids.
Self-loops are removed first, as `csr_graph` does; they change no answer below.

- G is **2-edge-connected** if n ≥ 2 and G − F is connected for every set F of at most one edge.
- G is **2-vertex-connected** if n ≥ 3 and G − X is connected for every set X of at most one
  vertex.

These are the usual definitions [Diestel, Section 1.4]. Two vertices joined by two parallel edges are
2-edge-connected but not 2-vertex-connected, and a single vertex is neither.

A **cycle** is a closed walk whose vertices are distinct apart from its first and last vertex, which
are the same. It has at least two edges; a cycle of two edges is a pair of parallel edges.

An **ear decomposition** of G is a sequence of walks P₁, …, P_k, the **ears**, each given by its
vertices v₀, …, v_ℓ (ℓ ≥ 1) and the edges between them, such that:

- **(E1)** P₁ is a cycle.
- **(E2)** For i ≥ 2, the ends v₀ and v_ℓ of P_i lie on earlier ears, and its inner vertices
  v₁, …, v_{ℓ−1} lie on no earlier ear and are distinct. So P_i is a path, or a cycle through one
  vertex of the earlier ears.
- **(E3)** Every edge of G lies on exactly one ear, and every vertex on at least one.

The decomposition is **open** if every ear after the first is a path with two different ends.

## 2. Soundness: what the checkers accept proves the answer

**Check for "no", 2-edge-connectivity.** Unless n < 2, the certificate is a set S of vertices. The
checker accepts if S is neither empty nor all of V, lists no vertex twice, and at most one edge
leaves it.

*Why it proves "no".* Let F be the set of edges that leave S. Then |F| ≤ 1, and in G − F no edge
leaves S. A path from a vertex in S to one outside S would need such an edge, so G − F is not
connected.

**Check for "no", 2-vertex-connectivity.** Unless n < 3, the certificate is a vertex set S and a
set X of at most one vertex, the separator. The checker accepts if S, X and R = V ∖ (S ∪ X) are
disjoint, S and R are not empty, and no edge joins S and R.

*Why it proves "no".* In G − X, a path from S to R would need an edge between S and R, since it
cannot pass through X. So G − X is not connected.

**Check for "yes".** The certificate is an ear decomposition, and for 2-vertex-connectivity an open
one. The checker accepts if the ears satisfy (E1)–(E3), and openness if required. It checks that
each listed edge exists and joins the two vertices it is listed between. G has no self-loops, so
the first ear has at least two edges. For 2-vertex-connectivity it also checks n ≥ 3.

**Theorem 3** [Robbins 1939]. If G has an ear decomposition, G is 2-edge-connected.

*Proof.* Let H_i be the union of the first i ears. We show by induction that H_i is connected and
that every edge of H_i lies on a cycle in H_i. For H₁, a cycle, this is clear. Let P_i have ends a
and b. If a = b, P_i is a cycle. Otherwise H_{i−1} contains a path Q from b to a, and since the
inner vertices of P_i are new, P_i followed by Q is a cycle. Either way the edges of P_i lie on a
cycle, and H_i is connected.

By (E3), H_k = G. G has at least the two vertices of P₁. Remove one edge e. The rest of a cycle
through e still joins its two ends, so G − e is connected. □

**Theorem 4** [Whitney 1932]. If G has an open ear decomposition and n ≥ 3, G is
2-vertex-connected.

*Proof.* We show by induction that each H_i is 2-vertex-connected or has exactly two vertices. H₁ is
a cycle. With two vertices it is of the second kind. With three or more, removing one vertex leaves
a path, which is connected.

Let P_i have the ends a ≠ b.

- If P_i has no inner vertex, it is one edge between existing vertices, and adding an edge keeps
  either property.
- If H_{i−1} has exactly the two vertices a and b, then P_i together with an edge of H_{i−1}
  between a and b is a cycle through all vertices of H_i. Removing any one vertex leaves a path
  through all the others.
- If H_{i−1} is 2-vertex-connected, remove a vertex z from H_i.
  - If z is an inner vertex of P_i, H_{i−1} stays connected, and the two pieces of P_i − z hang on
    a and on b.
  - If z lies in H_{i−1}, then H_{i−1} − z is connected, and P_i − z still contains a or b, so it
    hangs on H_{i−1} − z.

  H_i itself is connected as well.

By (E3), H_k = G, and n ≥ 3 rules out the second kind. □

**Running time.** The checkers read each listed vertex and edge once and stop at the first
violation. An accepted ear decomposition lists each edge once. Every ear needs an edge of its own,
so at most m + 1 ears are read before a violation appears. Hence O(n + m) time.

## 3. Completeness: the tests always find a certificate

The tests run the DFS and the chain decomposition, and then take the first case that applies.
Throughout, "the rest" is R = V ∖ (S ∪ X).

| Case | 2-edge-connectivity | 2-vertex-connectivity |
|------|---------------------|-----------------------|
| too few vertices | n < 2: no certificate needed | n < 3: no certificate needed |
| G disconnected | S = the first DFS tree | S = the first DFS tree, X = ∅ |
| a bridge t(x), x first in preorder | S = T(x) | deg(x) ≥ 2: S = T(x) ∖ {x}, X = {x}; else S = {x}, X = {p(x)} |
| a cycle chain C ≠ C₁ with first vertex v, ending with t(c) | (allowed: a closed ear) | S = T(c), X = {v} |
| none of these | yes: the chains | yes: the chains |

The DFS trees are the components of G, so no edge leaves the first tree. If G is disconnected,
there is a second tree, and R is not empty.

**Lemma 2 (bridges).** If t(x) is a bridge, t(x) is the only edge that leaves T(x). Every other
edge at x goes to a proper descendant of x.

*Proof.* The proof of (D2) shows that t(x) is a bridge iff no edge other than t(x) leaves T(x).
Let e ≠ t(x) be an edge at x. It does not leave T(x), so its other end is in T(x) ∖ {x}. □

So S = T(x) is an edge cut with one edge. For the separation, let deg(x) ≥ 2. By Lemma 2, x has a
second edge into T(x) ∖ {x}, so this set is not empty. Every edge from T(x) ∖ {x} to outside T(x)
would leave T(x), so it would be t(x), which does not have an end in T(x) ∖ {x}. Hence no edge
joins S = T(x) ∖ {x} to R, and R contains p(x). If deg(x) = 1, then T(x) = {x}, whose only edge
goes to p(x). R = V ∖ {x, p(x)} is not empty, because n ≥ 3.

**Lemma 3 (cycles at cut vertices).** Let C be a chain that is a cycle and not the C₁ of its tree.
Let v be its first vertex, and let t(c) be its last edge. Then no edge joins T(c) to
R = V ∖ (T(c) ∪ {v}), and R is not empty.

*Proof.* By (D1), an edge leaving T(c) is t(c) or a back edge (u, w) with w ∈ T(c) and u a proper
ancestor of c. If v is not a root, the proof of Theorem 2, case (b), shows that no such back edge
has u above v. So u = v. If v is a root, v is the only proper ancestor of c. So every edge leaving
T(c) ends at v.

R is not empty. If v is not a root, R contains the root of v's tree. If v is a root, the proof of
Theorem 2, case (b), gives a child c₁ ≠ c of v, and T(c₁) ⊆ R. □

**Lemma 4 (the chains are ears).** Let G be connected with n ≥ 2 and without bridges. Then the
chains, in the order in which they are found, form an ear decomposition of G. If G also has no cut
vertex, it is open.

*Proof.* We check (E1)–(E3). (E1) holds by (C3). There is only one tree, so C₁ is the first chain.

For (E2), let C be a later chain with first vertex v. When C is built, the marked vertices are the
first vertices of the chains built so far and the vertices that their walks passed. Each of them
lies on one of those chains.

- v lies on an earlier chain. If v is the root, C₁ starts at v. Otherwise t(v) is not a bridge, so
  by Theorem 1 some chain D contains it. D climbs from v to p(v), so its back edge covers v and
  starts at a proper ancestor of v. D is therefore built before every chain at v, and v lies on D.
- The walk of C passes only unmarked vertices before it stops, and it marks each one. So the inner
  vertices of C are on no earlier chain and are distinct.
- The walk stops at a marked vertex. That vertex lies on an earlier chain, or is v.

For (E3), every edge lies in exactly one chain by Theorem 1 and (C1). Every vertex has an edge,
because G is connected with n ≥ 2.

If G has no cut vertex, then by Theorem 2 (b) no chain other than C₁ is a cycle, so every later ear
is a path with two different ends. □

**Theorem 5.** The certificate that a test returns is accepted by its checker.

*Proof.*

- For 2-edge-connectivity, a graph that falls into none of the first three rows is connected, has
  n ≥ 2 and no bridge. Lemma 4 then gives an ear decomposition.
- For 2-vertex-connectivity, such a graph is in addition free of cycle chains other than C₁. By
  Theorem 2, it has no cut vertex.
  - In the fourth row, the graph has no bridge, so v is a cut vertex by Theorem 2 (b). Lemma 3
    gives the separation.
  - In the last row, Lemma 4 gives an open ear decomposition, and n ≥ 3.

The rows with a bridge are covered by Lemma 2 and the paragraph after it. □

## References

- R. Diestel. *Graph Theory.* 5th edition, Springer, 2017.
- R. M. McConnell, K. Mehlhorn, S. Näher, P. Schweitzer. *Certifying algorithms.* Computer
  Science Review 5(2):119–161, 2011.
- H. E. Robbins. *A theorem on graphs, with an application to a problem of traffic control.*
  American Mathematical Monthly 46(5):281–283, 1939.
- J. M. Schmidt. *A simple test on 2-vertex- and 2-edge-connectivity.* Information Processing
  Letters 113(7):241–244, 2013.
- H. Whitney. *Non-separable and planar graphs.* Transactions of the American Mathematical
  Society 34(2):339–362, 1932.
