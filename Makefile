all: siphash

siphash: siphash.o
	gcc -fopenmp -o siphash siphash.o

siphash.o: siphash.c
	gcc -c -fopenmp -O2 siphash.c

clean:
	rm -f siphash siphash.o
