cmake_minimum_required (VERSION 2.8)

add_subdirectory (updaterevision)
add_executable (botc
	src/Commands.cc
	src/DataBuffer.cc
	src/Events.cc
	src/Format.cc
	src/Lexer.cc
	src/LexerScanner.cc
	src/Main.cc
	src/Parser.cc
	src/String.cc
	src/StringTable.cc
	src/Variables.cc
)

get_target_property (UPDATEREVISION_EXE updaterevision LOCATION)

add_custom_target (revision_check ALL
    COMMAND ${UPDATEREVISION_EXE} src/GitInformation.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS updaterevision)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()