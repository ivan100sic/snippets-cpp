cmake_minimum_required(VERSION 3.0)

project(snippets-cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include_directories("${CMAKE_SOURCE_DIR}/include")

add_library(split "src/split.cpp")

add_executable(generate "src/generate.cpp")
target_link_libraries(generate split)

add_executable(test "src/test.cpp")
target_link_libraries(test split)
