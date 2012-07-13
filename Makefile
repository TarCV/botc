all:
	g++ -Wall -c -o scriptreader.o scriptreader.cxx
	g++ -Wall -c -o objwriter.o objwriter.cxx
	g++ -Wall -c -o str.o str.cxx
	g++ -Wall -c -o main.o main.cxx
	g++ -Wall -c -o parser.o parser.cxx
	g++ -Wall -c -o events.o events.cxx
	g++ -Wall -o botc scriptreader.o objwriter.o str.o main.o parser.o events.o

clean:
	rm -f *.o *~ botc
