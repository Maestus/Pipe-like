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
  struct conduct * conduit = conduct_open("ame");
  conduct_write(conduit, "la vie est trop courte", 22);
  char * buffer;
  buffer = malloc(22*sizeof(char));
  conduct_read(conduit, buffer, 22);
  return 0;
}
