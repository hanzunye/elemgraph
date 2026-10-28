// Reader for the graph files of the 9th DIMACS Implementation Challenge
// (shortest paths), such as the road network USA-road-d.NY.gr:
//
//     c <comment>
//     p sp <n> <m>
//     a <u> <v> <weight>        m arc lines; vertices are numbered from 1
//
// The files store every road as two opposite arcs. The reader ignores the
// weights and returns the canonical simple undirected graph (see
// canonicalize()), with vertices numbered from 0, so every road becomes one
// edge. Malformed input throws std::runtime_error naming the line.
#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <istream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "input_graph.hpp"

namespace elemgraph::gen {

namespace detail {

[[noreturn]] inline void dimacs_error(std::size_t line, const std::string& message) {
    throw std::runtime_error("DIMACS line " + std::to_string(line) + ": " + message);
}

inline std::vector<std::string_view> split_words(std::string_view text) {
    std::vector<std::string_view> words;
    const std::string_view blanks = " \t\r";
    std::size_t begin = text.find_first_not_of(blanks);
    while (begin != std::string_view::npos) {
        const std::size_t end = std::min(text.find_first_of(blanks, begin), text.size());
        words.push_back(text.substr(begin, end - begin));
        begin = text.find_first_not_of(blanks, end);
    }
    return words;
}

inline std::uint64_t parse_number(std::string_view word, std::size_t line) {
    std::uint64_t value = 0;
    const char* end = word.data() + word.size();
    const auto [stop, error] = std::from_chars(word.data(), end, value);
    if (error != std::errc{} || stop != end) {
        dimacs_error(line, "expected a non-negative number, got '" + std::string(word) + "'");
    }
    return value;
}

}  // namespace detail

inline input_graph read_dimacs(std::istream& in) {
    input_graph g;
    bool have_problem_line = false;
    std::uint64_t declared_arcs = 0;
    std::uint64_t arcs = 0;
    std::string text;
    std::size_t line = 0;
    while (std::getline(in, text)) {
        ++line;
        const auto words = detail::split_words(text);
        if (words.empty() || words[0] == "c") {
            continue;
        }
        if (words[0] == "p") {
            if (have_problem_line) {
                detail::dimacs_error(line, "second problem line");
            }
            if (words.size() != 4 || words[1] != "sp") {
                detail::dimacs_error(line, "expected 'p sp <n> <m>'");
            }
            const std::uint64_t n = detail::parse_number(words[2], line);
            declared_arcs = detail::parse_number(words[3], line);
            if (n >= std::numeric_limits<vertex>::max() ||
                declared_arcs > std::numeric_limits<edge_id>::max()) {
                detail::dimacs_error(line, "graph too large for 32-bit ids");
            }
            g.n = static_cast<vertex>(n);
            g.edges.reserve(declared_arcs);
            have_problem_line = true;
        } else if (words[0] == "a") {
            if (!have_problem_line) {
                detail::dimacs_error(line, "arc before the problem line");
            }
            if (words.size() != 4) {
                detail::dimacs_error(line, "expected 'a <u> <v> <weight>'");
            }
            const std::uint64_t u = detail::parse_number(words[1], line);
            const std::uint64_t v = detail::parse_number(words[2], line);
            detail::parse_number(words[3], line);  // the weight must be a number, but is not used
            if (u < 1 || u > g.n || v < 1 || v > g.n) {
                detail::dimacs_error(line, "vertex out of range 1.." + std::to_string(g.n));
            }
            g.edges.push_back({static_cast<vertex>(u - 1), static_cast<vertex>(v - 1)});
            ++arcs;
        } else {
            detail::dimacs_error(line, "unknown line type '" + std::string(words[0]) + "'");
        }
    }
    if (!have_problem_line) {
        throw std::runtime_error("DIMACS: no problem line");
    }
    if (arcs != declared_arcs) {
        throw std::runtime_error("DIMACS: problem line declares " + std::to_string(declared_arcs) +
                                 " arcs, file has " + std::to_string(arcs));
    }
    canonicalize(g);
    return g;
}

inline input_graph read_dimacs(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open " + path.string());
    }
    return read_dimacs(in);
}

}  // namespace elemgraph::gen
