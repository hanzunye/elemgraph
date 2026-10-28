cmake_minimum_required(VERSION 3.25)
project(elemgraph VERSION 0.1.0 LANGUAGES CXX)

# The library is header-only: an INTERFACE target that passes the include path
# and the C++20 requirement on to everything that links against it.
add_library(elemgraph INTERFACE)
add_library(elemgraph::elemgraph ALIAS elemgraph)
target_include_directories(elemgraph INTERFACE include)
target_compile_features(elemgraph INTERFACE cxx_std_20)

# Everything below is only for developing elemgraph itself, not for projects
# that pull it in with add_subdirectory().
if(NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

set(CMAKE_CXX_EXTENSIONS OFF)          # -std=c++20, not -std=gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # compile_commands.json for clangd

option(ELEMGRAPH_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

# Compiler flags for our own binaries (tests, later benchmarks). They are kept
# out of the elemgraph target so that users of the library don't inherit them.
add_library(elemgraph_build_flags INTERFACE)
target_compile_options(elemgraph_build_flags INTERFACE -Wall -Wextra -Wpedantic -Werror)

# Debug builds let the standard library check indices (vector, span, ...):
# libc++ hardening on macOS, libstdc++ assertions on Linux.
target_compile_definitions(elemgraph_build_flags INTERFACE
    $<$<CONFIG:Debug>:_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE>
    $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)

if(ELEMGRAPH_SANITIZE)
    # On macOS 26, ASan hangs while the program starts (seen with Apple clang 17
    # and LLVM 21), so macOS gets UBSan only. ASan runs on Linux.
    if(APPLE)
        set(sanitizers undefined)
        message(STATUS "ELEMGRAPH_SANITIZE: UBSan only on macOS (ASan hangs at startup)")
    else()
        set(sanitizers address,undefined)
    endif()
    target_compile_options(elemgraph_build_flags INTERFACE
        -fsanitize=${sanitizers} -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(elemgraph_build_flags INTERFACE -fsanitize=${sanitizers})
endif()

enable_testing()
add_subdirectory(tests)
