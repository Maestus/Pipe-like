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
  /*while(1){
    printf("va écrire\n");
  conduct_write(conduit, "aaa", 3);
  printf("ecris");
}*/
  void *buf0 = "short string\n";
  void *buf1 = "This is a longer string\n";
  void *buf2 = "This is the longest string in this example\n";
  int iovcnt;
  struct iovec iov[3];


  iov[0].iov_base = buf0;
  iov[0].iov_len = strlen(buf0);
  iov[1].iov_base = buf1;
  iov[1].iov_len = strlen(buf1);
  iov[2].iov_base = buf2;
  iov[2].iov_len = strlen(buf2);

  iovcnt = sizeof(iov) / sizeof(struct iovec);
  printf("J'ai ecris : %ld\n", conduct_writev(conduit, iov, iovcnt));

  return 0;
}
