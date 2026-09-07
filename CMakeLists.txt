cmake_minimum_required(VERSION 3.20)
project(aptenodytes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(APTEN_BUILD_TESTS "Build C++ tests" ON)
option(APTEN_BUILD_PYTHON "Build Python bindings (nanobind)" ON)

# --- Core library (header + src) ---
add_library(apten_core
    engine/src/portfolio.cpp
    engine/src/bar_feed.cpp
    engine/src/metrics.cpp
    engine/src/engine.cpp
    engine/src/execution_model.cpp
)
target_include_directories(apten_core PUBLIC engine/include)
target_compile_options(apten_core PRIVATE -Wall -Wextra -Wpedantic)

# --- Python bindings (nanobind) ---
if(APTEN_BUILD_PYTHON)
    find_package(Python 3.10 COMPONENTS Interpreter Development.Module REQUIRED)
    # Fetch nanobind via FetchContent (pinned)
    include(FetchContent)
    FetchContent_Declare(
        nanobind
        GIT_REPOSITORY https://github.com/wjakob/nanobind.git
        GIT_TAG v2.4.0
    )
    FetchContent_MakeAvailable(nanobind)
    nanobind_add_module(_apten_core
        python/src/bindings.cpp
    )
    target_link_libraries(_apten_core PRIVATE apten_core)
    # Install to python/backtester so `import backtester._apten_core` works during dev
    install(TARGETS _apten_core LIBRARY DESTINATION python/backtester)
endif()

# --- C++ tests (GoogleTest via FetchContent) ---
if(APTEN_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(apten_tests
        engine/tests/test_portfolio.cpp
        engine/tests/test_metrics.cpp
        engine/tests/test_engine.cpp
    )
    target_link_libraries(apten_tests PRIVATE apten_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(apten_tests)
endif()
