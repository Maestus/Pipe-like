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
  struct conduct * conduit = conduct_create("ame", 10, 100);
  printf("Le nom : %s\n", conduit->name);
  char * buffer;
  buffer = malloc(22*sizeof(char));
  conduct_write(conduit, "la vie est trop longue", 22);
  while(1)
    conduct_read(conduit, buffer, 22);
  return 0;
}
