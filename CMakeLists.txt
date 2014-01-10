add_executable (botc
	src/commands.cxx
	src/events.cxx
	src/format.cc
	src/main.cxx
	src/objwriter.cxx
	src/parser.cxx
	src/preprocessor.cxx
	src/scriptreader.cxx
	src/str.cxx
	src/stringtable.cxx
	src/variables.cxx
)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -W -Wall")