cmake_minimum_required(VERSION 3.10)

project(DijkstraMPI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

find_package(MPI REQUIRED COMPONENTS CXX)

add_executable(DijkstraMPI ${SOURCES})

target_include_directories(DijkstraMPI PRIVATE include)
target_link_libraries(DijkstraMPI PRIVATE MPI::MPI_CXX)