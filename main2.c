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
  /*struct conduct * conduit = conduct_open("file");
  if(conduit == NULL){
    printf("%s\n", strerror(errno));
    return 1;
  }
  //while(1){
    //printf("life\n");
void * buff = malloc(6);
for (size_t i = 0; i < 6; i++) {
  strcat(buff, "a");
}*/
char * buffer = malloc(100);
void * a = &buffer;
printf("%ld\n", sizeof(a));
/*while(1){
    //sleep(1);
    printf("retourne : %ld\n",conduct_write(conduit, buff,6));
    //printf("retourne : %ld\n",conduct_write(conduit, "d",1));
    //printf("retourne : %ld\n",conduct_write(conduit, "eeeee",5));

    //char * buff = malloc(conduit->capacity*sizeof(char));
    //conduct_read(conduit, buff, conduit->capacity);
    //printf("[%s]\n", buff);
    //printf("is strange\n");
    //sleep(1);
    //conduct_write(conduit, "ppppppp",7);
  //}
}*/
  return 0;
}
