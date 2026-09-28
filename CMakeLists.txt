
cmake_minimum_required(VERSION 3.28)
                
project("GLTFScene")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

include(cmake/CPM.cmake)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

cpmaddpackage("gh:zeux/meshoptimizer@1.3")

add_subdirectory("external")

add_subdirectory("src")

add_subdirectory("tests")
