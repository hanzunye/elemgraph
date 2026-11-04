// Disjoint sets over the vertices 0 .. n-1, with union by size and path
// halving. A sequence of m operations takes O(m α(n)) time [Tarjan 1975].
#pragma once

#include <cassert>
#include <numeric>
#include <utility>
#include <vector>

#include "../edge_list.hpp"

namespace elemgraph {

class union_find {
public:
    // n sets with one vertex each.
    explicit union_find(vertex n = 0) { reset(n); }

    // Starts again with n sets of one vertex each, reusing the memory.
    void reset(vertex n) {
        parent_.resize(n);
        std::iota(parent_.begin(), parent_.end(), vertex{0});
        size_.assign(n, 1);
    }

    // The representative of the set that contains x. Path halving: every
    // vertex on the way to the root is linked to its grandparent, which
    // roughly halves the path for the next search.
    [[nodiscard]] vertex find(vertex x) {
        assert(x < parent_.size());
        while (parent_[x] != x) {
            parent_[x] = parent_[parent_[x]];
            x = parent_[x];
        }
        return x;
    }

    // Merges the sets of a and b. Returns false if they were already one set.
    bool unite(vertex a, vertex b) {
        a = find(a);
        b = find(b);
        if (a == b) {
            return false;
        }
        // Union by size: the smaller tree goes below the larger, so no tree
        // gets deeper than log2(n).
        if (size_[a] < size_[b]) {
            std::swap(a, b);
        }
        parent_[b] = a;
        size_[a] += size_[b];
        return true;
    }

private:
    std::vector<vertex> parent_;  // parent_[x] == x for representatives
    std::vector<vertex> size_;    // size_[r]: the number of vertices in the set of representative r
};

}  // namespace elemgraph
