cmake_minimum_required (VERSION 2.8)

set (BOTC_HEADERS
	src/botStuff.h
	src/commands.h
	src/list.h
	src/dataBuffer.h
	src/events.h
	src/expression.h
	src/format.h
	src/lexer.h
	src/lexerScanner.h
	src/macros.h
	src/main.h
	src/parser.h
	src/property.h
	src/string.h
	src/stringTable.h
	src/tokens.h
	src/types.h
)

set (BOTC_SOURCES
	src/commands.cpp
	src/dataBuffer.cpp
	src/events.cpp
	src/expression.cpp
	src/format.cpp
	src/lexer.cpp
	src/lexerScanner.cpp
	src/main.cpp
	src/parser.cpp
	src/string.cpp
	src/stringTable.cpp
)

add_subdirectory (updaterevision)
add_subdirectory (namedenums)

get_target_property (UPDATEREVISION_EXE updaterevision LOCATION)

add_custom_target (revision_check ALL
    COMMAND ${UPDATEREVISION_EXE} src/gitinfo.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS updaterevision)

get_target_property (NAMEDENUMS_EXE namedenums LOCATION)

add_custom_target (botc_enum_strings ALL
    COMMAND ${NAMEDENUMS_EXE} ${BOTC_HEADERS} src/enumStrings.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS namedenums)

add_executable (botc ${BOTC_SOURCES})
add_dependencies(botc revision_check botc_enum_strings)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()
