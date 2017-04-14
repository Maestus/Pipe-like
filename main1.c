#include "conduct.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
  struct conduct * conduit = conduct_create("file", 10, 20);
  printf("[Le nom] : %s\n", conduit->name);
  conduct_write(conduit, "abcdef", 6);
  while(1){
    printf("=%d\n", conduit->remplissage);
      sleep(1);
      char * buff = malloc(conduit->capacity*sizeof(char));
      conduct_read(conduit, buff, 4);
      printf("[%s]\n", buff);
  }
  return 0;
}
