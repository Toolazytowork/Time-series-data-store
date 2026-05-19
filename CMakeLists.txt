cmake_minimum_required(VERSION 3.14)
project(TSDB VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(tsdb_core src/TSDB.cpp src/WALWriter.cpp src/WALReader.cpp src/WALCompactor.cpp src/BitStreamWriter.cpp src/BitStreamReader.cpp)
target_include_directories(tsdb_core PUBLIC src)

add_executable(tsdb_tests tests/test_basic.cpp)
target_link_libraries(tsdb_tests tsdb_core gtest_main)

include(GoogleTest)
gtest_discover_tests(tsdb_tests)
