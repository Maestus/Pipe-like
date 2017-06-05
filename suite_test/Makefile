bench: bench.c ../conduct.o
	gcc -Wall bench.c ../conduct.o -o bench -pthread

clean:
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" -delete
