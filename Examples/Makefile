all:
	make transfert
	make non_block

transfert: transfert.c ../conduct.o
	gcc -g -Wall `pkg-config --cflags gtk+-3.0` transfert.c ../conduct.o `pkg-config --libs gtk+-3.0` -o transfert

non_block: non_block.c ../conduct.o
	gcc -g -Wall non_block.c ../conduct.o -o non_block -pthread

clean:
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" -delete
