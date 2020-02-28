cmake_minimum_required(VERSION 3.10)
project(vitamine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-parentheses -march=native")

include_directories(source)
include_directories(${CMAKE_BINARY_DIR})

set(GENDIR "${CMAKE_BINARY_DIR}/generated")
set(GENERATED_FILES "${GENDIR}/ids.hpp")

file(GLOB_RECURSE DATAGEN_FILES "datagen/*")
add_custom_command(COMMAND python3 genheaders.py ${CMAKE_BINARY_DIR}
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/datagen
                   DEPENDS ${DATAGEN_FILES}
                   OUTPUT ${GENERATED_FILES}
                   COMMENT "Generating constants ...")

file(GLOB_RECURSE COMMON_FILES "source/common/*.[ch]pp")

file(GLOB_RECURSE PROXYD_FILES "source/proxyd/*.[ch]pp")
add_executable(vitaproxyd ${PROXYD_FILES} ${COMMON_FILES} ${GENERATED_FILES})
target_link_libraries(vitaproxyd boost_system pthread)
