all:
	make main1
	make main2
	make main3
	make main4

main1: main1.c ../conduct.o
	gcc -Wall main1.c ../conduct.o -o main1 -pthread

main2: main2.c ../conduct.o
	gcc -Wall main2.c ../conduct.o -o main2 -pthread

main3: main3.c ../conduct.o
	gcc -Wall main3.c ../conduct.o -o main3 -pthread

main4: main4.c ../conduct.o
	gcc -Wall main4.c ../conduct.o -o main4 -pthread

clean:
	find . -maxdepth 1 -type f  ! -name "*.c" ! -name "*.h" ! -name "Makefile" -delete
