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
  struct conduct * conduit = conduct_open("file");
  while(1){
    printf("va écrire\n");
  conduct_write(conduit, "aaa", 3);
  printf("ecris");
  }
  return 0;
}
