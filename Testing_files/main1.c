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
  struct conduct * conduit;
  if((conduit = conduct_create(NULL, 5, 100)) == NULL){
    perror("main1 creation");
    exit(1);
  }

  void *buf0 = "short string\n";
  void *read = malloc(7);

  if(fork()){
    conduct_write(conduit, buf0, 7);
  } else {
    conduct_read(conduit, read, 7);
    printf("%s\n", (char *)read);
  }
  //try_conduct_write(conduit, );

  /*if(conduit == NULL) {
    printf("%s\n", strerror(errno));
  } else {
    --while(1){
      sleep(1);
        printf("Remplissage = %d\n", conduit->remplissage);
        printf("Lecture = %d\n", conduit->lecture);
        char * buff = malloc(conduit->capacity*sizeof(char));
        printf("j'ai lu : %ld",conduct_read(conduit, buff, 20));
        printf("[%s]\n", buff);
    }--
    int iovcnt;
    struct iovec iov[3];

    while (1) {

      iov[0].iov_base = malloc(10);
      iov[0].iov_len = 10;
      iov[1].iov_base = malloc(5);
      iov[1].iov_len = 5;
      iov[2].iov_base = malloc(2);
      iov[2].iov_len = 2;

      iovcnt = sizeof(iov) / sizeof(struct iovec);

      ssize_t nb_c = conduct_readv(conduit, iov, iovcnt);
      void * buff = malloc(nb_c);

      size_t reader = nb_c;
      for (size_t i = 0; i < iovcnt; i++) {
          size_t read = (reader >= iov[i].iov_len) ? iov[i].iov_len : reader;
          //printf("reader : %ld\n", read);
          memcpy(buff, (void *) iov[i].iov_base,  read);
          //printf("%s\n",(char *) iov[i].iov_base);
          reader -= read;
          buff += read;
          if(nb_c == 0)
            break;
      }

      printf("J'ai lus : |%s|\n",(char *) (buff-nb_c));
      free(iov[0].iov_base);
      free(iov[1].iov_base);
      free(iov[2].iov_base);
      sleep(1);
    }
  }*/
  return 0;
}
