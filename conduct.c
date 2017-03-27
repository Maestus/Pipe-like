#include "conduct.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

struct conduct *conduct_create(const char *name, size_t a, size_t c){
  void * mp_d;
  int fd;
  struct conduct * conduit;
  conduit = malloc(sizeof(struct conduct));

  if ( name != NULL) {
    if((fd = shm_open(name, O_CREAT | O_RDWR, 0666)) == -1){
        printf("shm_open : failed in main");
        return NULL;
    }

    if (ftruncate(fd, sizeof(struct conduct)) == -1){
      printf("ftruncate failed : %s\n", strerror(errno));
      return NULL;
    }

    if ((mp_d = mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
      printf("mmap failed : %s\n", strerror(errno));
      return NULL;
    }

    conduit = mp_d;
    conduit->is_anon = 0;

    conduit->name = malloc(15*sizeof(char));
    strcpy(conduit->name, name);

  } else {
    conduit->is_anon = 1;
  }

  conduit->capacity = c;
  conduit->atomic = a;

  return conduit;
}

struct conduct *conduct_open(const char *name){
  int fd;
  void * mp_d;
  struct conduct * conduit;
  conduit = malloc(sizeof(struct conduct));

  if((fd = shm_open(name, O_RDWR, 0666)) == -1){
      printf("shm_open : failed in main");
      return NULL;
  }

  if ((mp_d = mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
    printf("mmap failed : %s\n", strerror(errno));
    return NULL;
  }

  conduit = mp_d;
  return conduit;
}

void conduct_close(struct conduct * conduit){

}

void conduct_destroy(struct conduct * conduit){

}

int main(int argc, char const *argv[]) {
  struct conduct * conduit = conduct_create("ame", 10, 100);
  printf("Le nom : %s\n", conduit->name);
  struct conduct * conduit2 = conduct_open("ame");
  printf("Le taille : %zu\n", conduit2->capacity);
  return 0;
}
