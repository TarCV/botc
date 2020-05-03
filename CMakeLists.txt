cmake_minimum_required (VERSION 2.8)

set (BOTC_HEADERS
	src/botStuff.h
	src/commandline.h
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
	src/stringClass.h
	src/stringTable.h
	src/tokens.h
	src/types.h
)

set (BOTC_SOURCES
	src/commandline.cpp
	src/commands.cpp
	src/dataBuffer.cpp
	src/events.cpp
	src/expression.cpp
	src/format.cpp
	src/lexer.cpp
	src/lexerScanner.cpp
	src/main.cpp
	src/parser.cpp
	src/stringClass.cpp
	src/stringTable.cpp
		)

add_subdirectory (updaterevision)
add_subdirectory (namedenums)
get_target_property (UPDATEREVISION_EXE updaterevision LOCATION)
get_target_property (NAMEDENUMS_EXE namedenums LOCATION)

add_custom_target (revision_check ALL
    COMMAND ${UPDATEREVISION_EXE} ${CMAKE_BINARY_DIR}/gitinfo.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS updaterevision)

add_custom_target (enumstrings ALL
    BYPRODUCTS enumstrings.cpp
    COMMAND ${NAMEDENUMS_EXE} ${BOTC_HEADERS}
		${CMAKE_BINARY_DIR}/enumstrings.h
		${CMAKE_BINARY_DIR}/enumstrings.cpp
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS namedenums)

add_executable (botc ${BOTC_SOURCES} ${CMAKE_BINARY_DIR}/enumstrings.cpp)
add_dependencies (botc revision_check enumstrings)
include_directories (${CMAKE_BINARY_DIR})
include_directories (${CMAKE_SOURCE_DIR}/src)

set_target_properties(botc PROPERTIES CXX_STANDARD 11)
if (NOT MSVC)
	target_compile_options(botc PRIVATE -W -Wall)

	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
		target_compile_options(botc PRIVATE -DDEBUG)
	endif()
endif()
if (MSVC)
	target_compile_options(botc PRIVATE /Zc:__cplusplus)
endif()
