# Based on: https://llvm.org/docs/CMake.html#embedding-llvm-in-your-project
cmake_minimum_required(VERSION 3.13.4)
project(solid_lang)

set(CMAKE_CXX_STANDARD 17)
set(LLVM_DIR /usr/local/opt/llvm@15/lib/cmake/llvm/) # Update this based on your llvm installation

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(solid_lang main.cpp Lexer.cpp Lexer.h Expression.h Parser.cpp Parser.h)

llvm_map_components_to_libnames(llvm_libs core)
target_link_libraries(solid_lang ${llvm_libs})