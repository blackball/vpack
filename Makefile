CC=gcc

vpack-example: example.o vpack.o 
	$(CC) example.o vpack.o -o vpack-example

vpack.o: vpack.c
	$(CC) -c vpack.c

example.o: example.c
	$(CC) -c example.c

clean: 
	rm -rf *.o vpack-example