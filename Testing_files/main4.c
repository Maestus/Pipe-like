#include "../conduct.h"
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
  void * buff = malloc(100);
  conduct_read(conduit, buff, 100);
  printf("%s", (char *) buff);
  conduct_destroy(conduit);
  return 0;
}
