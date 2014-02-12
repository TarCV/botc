cmake_minimum_required (VERSION 2.4)
add_executable (namedenums NamedEnumerations.cc)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")
