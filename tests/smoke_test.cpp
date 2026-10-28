// Checks the toolchain: a GoogleTest binary builds in C++20 mode and runs.
// Remove this file once the first real tests exist.
#include <gtest/gtest.h>

static_assert(__cplusplus >= 202002L, "elemgraph requires C++20");

TEST(Smoke, TestBinaryRuns) {
    SUCCEED();
}
