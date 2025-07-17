cmake_minimum_required(VERSION 4.0)
set(NAME "CSEngine")
project("${NAME}" VERSION 0.0.0)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependency.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Clang.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Target.cmake")
