cmake_minimum_required (VERSION 2.8)

set (BOTC_HEADERS
	src/BotStuff.h
	src/Commands.h
	src/Containers.h
	src/DataBuffer.h
	src/Events.h
	src/Expression.h
	src/Format.h
	src/Lexer.h
	src/LexerScanner.h
	src/Macros.h
	src/Main.h
	src/Parser.h
	src/Property.h
	src/String.h
	src/StringTable.h
	src/Tokens.h
	src/Types.h
)

set (BOTC_SOURCES
	src/Commands.cc
	src/DataBuffer.cc
	src/Events.cc
	src/Expression.cc
	src/Format.cc
	src/Lexer.cc
	src/LexerScanner.cc
	src/Main.cc
	src/Parser.cc
	src/String.cc
	src/StringTable.cc
)

add_subdirectory (updaterevision)
add_subdirectory (namedenums)

get_target_property (UPDATEREVISION_EXE updaterevision LOCATION)

add_custom_target (revision_check ALL
    COMMAND ${UPDATEREVISION_EXE} src/GitInformation.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS updaterevision)

get_target_property (NAMEDENUMS_EXE namedenums LOCATION)

add_custom_target (botc_enum_strings ALL
    COMMAND ${NAMEDENUMS_EXE} ${BOTC_HEADERS} src/EnumStrings.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS namedenums)

add_executable (botc ${BOTC_SOURCES})
add_dependencies(botc revision_check botc_enum_strings)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()