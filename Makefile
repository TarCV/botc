all:
	g++ -Wall -c -o scriptreader.o scriptreader.cxx
	g++ -Wall -c -o objwriter.o objwriter.cxx
	g++ -Wall -c -o str.o str.cxx
	g++ -Wall -c -o main.o main.cxx
	g++ -Wall -c -o parser.o parser.cxx
	g++ -Wall -c -o events.o events.cxx
	g++ -Wall -c -o commands.o commands.cxx
	g++ -Wall -c -o stringtable.o stringtable.cxx
	g++ -Wall -c -o variables.o variables.cxx
	g++ -Wall -c -o preprocessor.o preprocessor.cxx
	g++ -Wall -o botc scriptreader.o objwriter.o str.o main.o parser.o events.o \
		commands.o stringtable.o variables.o preprocessor.o

clean:
	rm -f *.o *~ botc
