all:
	make conduct
	make julia

conduct: conduct.h conduct.c
	gcc -Wall -c conduct.h conduct.c -c

#main1: main1.c conduct.o
#	gcc -Wall main1.c conduct.o -o main1 -pthread

#main2: main2.c conduct.o
#	gcc -Wall main2.c conduct.o -o main2 -pthread*/

julia:
	gcc -g -O3 -ffast-math -Wall -pthread `pkg-config --cflags gtk+-3.0` julia.c conduct.c `pkg-config --libs gtk+-3.0` -lm

clean:
	rm main1
	rm main2
	rm *.o
	rm *.gch
	rm a.out
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" -delete
