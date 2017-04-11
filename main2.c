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
  //void * buff = malloc(6*sizeof(char));
  //conduct_read(conduit, buff, 6);
  //while(1){}
  //strncat(buff, conduit->buffer, 6);
  //printf("=%d\n", conduit->remplissage);
  conduct_write(conduit, "olelleh",7);
  //conduct_write(conduit, "la vie est trop courte", 22);
  // Write it now to disk
  /*if (msync(conduit->buffer, conduit->capacity, MS_SYNC) == -1)
  {
      perror("Could not sync the file to disk");
  }*/
  while(1){}
  return 0;
}
