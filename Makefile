output: main.o
	g++ main.o -o uctp -g -W
	./uctp

main.o: main.cpp
	g++ -c main.cpp

clean:
	rm *.o uctp

ex:
	./uctp