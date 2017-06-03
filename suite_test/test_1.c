/**
Test d'ecriture dans un conduit / pipe
**/

#include "../conduct.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


char * generate(int length){
  char * list = malloc(length);
  char randomletter;
  for (size_t i = 0; i < length; i++) {
    randomletter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[random () % 26];
    list[i] = randomletter;
  }
  return list;
}


int main(int argc, char const *argv[]) {

  //Initialisation

  struct timespec start, stop;

  char * test1 = generate(100);
  char * test3 = generate(1000);
  char * test5 = generate(10000);

  struct conduct * conduit = conduct_create(NULL, 1000, 100000);
  int ret = 0;
  void * buffer;

  int com[2];
  pipe(com);

  //Test 1

  buffer = malloc(100);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  try_conduct_write(conduit, test1, 100);
  try_conduct_read(conduit, buffer, 100);

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[conduit] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  free(buffer);
  buffer = malloc(100);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  write(com[1], test1, 100);
  read(com[0], buffer, 100);

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[pipe] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  free(buffer);

  //Test 2

  buffer = malloc(1000);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  while(ret < 1000){
    ret += try_conduct_write(conduit, test3+ret, 1000);
    try_conduct_read(conduit, buffer, ret);
  }

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[conduit] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  ret = 0;
  free(buffer);
  buffer = malloc(1000);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  write(com[1], test3, 1000);
  read(com[0], buffer, 1000);

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[pipe] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  free(buffer);

  //Test 3

  buffer = malloc(10000);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  while(ret < 10000){
    ret += try_conduct_write(conduit, test5+ret, 1000);
    try_conduct_read(conduit, buffer, ret);
  }

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[conduit] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  ret = 0;
  free(buffer);
  buffer = malloc(10000);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  write(com[1], test5, 10000);
  read(com[0], buffer, 10000);

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[pipe] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  free(buffer);

  return 0;
}
