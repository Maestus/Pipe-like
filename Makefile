all:
	make conduct
	make main1
	make main2
	make main3
	make julia

conduct: conduct.h conduct.c
	gcc -Wall -c conduct.h conduct.c -c

main1: main1.c conduct.o
	gcc -Wall main1.c conduct.o -o main1 -pthread

main2: main2.c conduct.o
	gcc -Wall main2.c conduct.o -o main2 -pthread

main3: main3.c conduct.o
	gcc -Wall main3.c conduct.o -o main3 -pthread

julia:
	gcc -g -O3 -ffast-math -Wall -pthread `pkg-config --cflags gtk+-3.0` julia.c conduct.c `pkg-config --libs gtk+-3.0` -lm

clean:
	rm *.o
	rm *.gch
	rm a.out
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" -delete
