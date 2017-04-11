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
  printf("[Le nom] : %s, %ld\n", conduit->name, sysconf(_SC_PAGE_SIZE));
  // char * buff = malloc(22*sizeof(char));
  conduct_write(conduit, "aaaaaa", 6);
  // conduct_read(conduit, buff, 22);
  while(1){
      sleep(1);
      char * buff = malloc(conduit->capacity*sizeof(char));
      conduct_read(conduit, buff, 3);
  }
  return 0;
}
