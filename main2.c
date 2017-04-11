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
  conduct_write(conduit, "olelleh",7);
  for(int i=0;i<3;i++){
      conduct_write(conduit, "olelleh",7);
  }

  return 0;
}
