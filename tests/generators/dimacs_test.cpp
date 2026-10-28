#include "generators/dimacs.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace elemgraph::gen {
namespace {

using ::testing::HasSubstr;
using ::testing::ThrowsMessage;

input_graph parse(const std::string& text) {
    std::istringstream in(text);
    return read_dimacs(in);
}

TEST(Dimacs, ReadsOppositeArcsAsOneEdge) {
    const input_graph g = parse(
        "c a small road network\n"
        "p sp 4 6\n"
        "a 1 2 7\n"
        "a 2 1 7\n"
        "a 2 3 1\n"
        "a 3 2 1\n"
        "a 3 3 5\n"  // self-loop: removed
        "a 4 1 9\n");
    EXPECT_EQ(g.n, 4u);
    EXPECT_EQ(g.edges, (std::vector<edge>{{0, 1}, {0, 3}, {1, 2}}));
}

TEST(Dimacs, AcceptsBlankLinesTabsAndWindowsLineEnds) {
    const input_graph g = parse("p sp 2 1\r\n\n\ta\t1   2 3\r\n");
    EXPECT_EQ(g.edges, (std::vector<edge>{{0, 1}}));
}

TEST(Dimacs, ReportsMalformedInputWithTheLineNumber) {
    const auto fails_with = [](const std::string& message) {
        return ThrowsMessage<std::runtime_error>(HasSubstr(message));
    };
    EXPECT_THAT([] { parse("a 1 2 3\n"); }, fails_with("line 1: arc before the problem line"));
    EXPECT_THAT([] { parse("p sp 2 1\np sp 2 1\n"); }, fails_with("line 2: second problem line"));
    EXPECT_THAT([] { parse("p max 2 1\n"); }, fails_with("line 1: expected 'p sp <n> <m>'"));
    EXPECT_THAT([] { parse("p sp 2 1\na 1 3 1\n"); }, fails_with("line 2: vertex out of range 1..2"));
    EXPECT_THAT([] { parse("p sp 2 1\na 0 1 1\n"); }, fails_with("line 2: vertex out of range"));
    EXPECT_THAT([] { parse("p sp 2 1\na 1 x 1\n"); }, fails_with("line 2: expected a non-negative number, got 'x'"));
    EXPECT_THAT([] { parse("p sp 2 1\na 1 2 -4\n"); }, fails_with("line 2: expected a non-negative number"));
    EXPECT_THAT([] { parse("p sp 2 1\na 1 2\n"); }, fails_with("line 2: expected 'a <u> <v> <weight>'"));
    EXPECT_THAT([] { parse("p sp 2 1\ne 1 2\n"); }, fails_with("line 2: unknown line type 'e'"));
    EXPECT_THAT([] { parse("c only a comment\n"); }, fails_with("no problem line"));
    EXPECT_THAT([] { parse("p sp 2 2\na 1 2 1\n"); }, fails_with("declares 2 arcs, file has 1"));
}

TEST(Dimacs, ReportsMissingFile) {
    EXPECT_THAT([] { read_dimacs(std::filesystem::path("no/such/file.gr")); },
                ThrowsMessage<std::runtime_error>(HasSubstr("cannot open no/such/file.gr")));
}

}  // namespace
}  // namespace elemgraph::gen
