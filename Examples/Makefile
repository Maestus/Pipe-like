all:
	gcc -g -Wall `pkg-config --cflags gtk+-3.0` transfert.c `pkg-config --libs gtk+-3.0` -o transfert

clean:
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" -delete
