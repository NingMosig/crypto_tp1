all: siphash

siphash: siphash.o
	gcc -o siphash siphash.o

triangular_matrix.o: siphash.c
	gcc -c -O2 siphash.c

clean:
	rm -f siphash siphash.o

test:
	echo -e "\ntests\n"
