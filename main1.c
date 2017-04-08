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
  struct conduct * conduit = conduct_create("file", 10, 1000);
  printf("[Le nom] : %s\n", conduit->name);
  char * buff = malloc(22*sizeof(char));
  conduct_read(conduit, buff, 22);
  return 0;
}
