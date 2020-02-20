cmake_minimum_required(VERSION 3.10)
project(vitamine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-parentheses -march=native")

file(GLOB_RECURSE COMMON_FILES "source/common/*.[ch]pp")

file(GLOB_RECURSE PROXYD_FILES "source/proxyd/*.[ch]pp")
add_executable(vitaproxyd ${PROXYD_FILES} ${COMMON_FILES})
target_include_directories(vitaproxyd PRIVATE source)
target_link_libraries(vitaproxyd boost_system pthread)
