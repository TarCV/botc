cmake_minimum_required (VERSION 2.8)

add_subdirectory (updaterevision)
add_executable (botc
	src/commands.cc
	src/data_buffer.cc
	src/events.cc
	src/format.cc
	src/lexer.cc
	src/lexer_scanner.cc
	src/main.cc
	src/parser.cc
	src/str.cc
	src/stringtable.cc
	src/variables.cc
)

get_target_property (UPDATEREVISION_EXE updaterevision LOCATION)

add_custom_target (revision_check ALL
    COMMAND ${UPDATEREVISION_EXE} src/gitinfo.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS updaterevision)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()