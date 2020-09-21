# makefile
output: main.o
	g++ -g -w -std=c++11  main.o -o shell
main.o: main.cpp
	g++ -g -w -std=c++11  -c main.cpp

clean:
	rm -rf *.o *.csv  output *.txt shell a b
