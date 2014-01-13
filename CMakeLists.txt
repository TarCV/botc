add_executable (botc
	src/commands.cc
	src/data_buffer.cc
	src/events.cc
	src/format.cc
	src/lexer.cc
	src/lexer_scanner.cc
	src/main.cc
	src/object_writer.cc
	src/parser.cc
	src/str.cc
	src/stringtable.cc
	src/variables.cc
)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")