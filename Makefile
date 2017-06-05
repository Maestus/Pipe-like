all:
	make conduct
	make julia
	make -C Testing_files
	make -C Examples
	make -C suite_test

conduct: conduct.h conduct.c
	gcc -g -Wall -c conduct.h conduct.c -c

julia:
	gcc -g -O3 -ffast-math -Wall -pthread `pkg-config --cflags gtk+-3.0` julia.c conduct.c `pkg-config --libs gtk+-3.0` -lm

clean:
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" ! -name "*.odt" -delete
	make clean -C Testing_files
	make clean -C Examples
	make clean -C suite_test
