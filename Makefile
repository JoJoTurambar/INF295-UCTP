output: main.o
	g++ main.o -o uctp -g -W
	./uctp

main.o: main.cpp
	g++ -c main.cpp -g -W

clean:
	rm *.o uctp

ex:
	./uctp

gdb:
	g++ -g main.cpp -o uctp -W
	gdb ./uctp