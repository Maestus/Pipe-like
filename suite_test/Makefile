bench: bench.c ../conduct.o
	gcc -g -Wall bench.c ../conduct.o -o bench -pthread

clean:
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" ! -name "*.pdf" ! -name "*.ods" -delete
