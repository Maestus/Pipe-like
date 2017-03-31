#include "conduct.h"
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>


const char * generate_name(){
  char *name;
  name = malloc(15 * sizeof(char));
  int rnd = rand()%99999 + 1;
  sprintf(name, "%s_%d", "conduct", rnd);
  return name;
}

struct conduct *conduct_create(const char *name, size_t a, size_t c){
  int fd;
  struct conduct * conduit;

  if ( name != NULL) {
    if((fd = open(name, O_CREAT | O_RDWR, 0666)) == -1){
        printf("shm_open : failed in main");
        return NULL;
    }

    if (ftruncate(fd, sizeof(struct conduct)) == -1){
      printf("ftruncate failed : %s\n", strerror(errno));
      return NULL;
    }

    if ((conduit = mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
      printf("mmap failed : %s\n", strerror(errno));
      return NULL;
    }

    conduit->is_anon = 0;

    conduit->name = malloc(15*sizeof(char));
    strcpy(conduit->name, name);
    conduit->capacity = c;
    if((conduit->buffer = mmap(conduit + 1, conduit->capacity*sizeof(char), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) == (void *) -1){
      printf("mmap failed : %s\n", strerror(errno));
      return NULL;
    }

  } else {

    if((fd = shm_open(generate_name(), O_CREAT | O_RDWR, 0666)) == -1){
        printf("shm_open : failed in main");
        return NULL;
    }

    if (ftruncate(fd, sizeof(struct conduct)) == -1){
      printf("ftruncate failed : %s\n", strerror(errno));
      return NULL;
    }

    if ((conduit = mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
      printf("mmap failed : %s\n", strerror(errno));
      return NULL;
    }

    conduit->is_anon = 1;

    conduit->name = malloc(15*sizeof(char));
    strcpy(conduit->name, name);
    conduit->capacity = c;
    conduit->buffer = mmap(conduit + 1, conduit->capacity*sizeof(char), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
  }
  conduit->atomic = a;
  conduit->lecture = 0;
  conduit->ecriture = 0;
  conduit->remplissage = 0;
  return conduit;
}

struct conduct *conduct_open(const char *name){
  int fd;
  struct conduct * conduit;

  if((fd = open(name, O_RDWR, 0666)) == -1){
      printf("shm_open : failed in main");
      return NULL;
  }

  if ((conduit = mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
    printf("mmap failed : %s\n", strerror(errno));
    return NULL;
  }

  conduit->buffer = mmap(conduit + 1, conduit->capacity*sizeof(char), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);

  return conduit;
}

ssize_t conduct_read(struct conduct * conduit, void * buff, size_t count){
  while(conduit->lecture == conduit->ecriture) {}
  if(conduit->lecture < conduit->capacity){
    for (size_t i = 0; i < count; i++) {
      printf("%c", conduit->buffer[i]);
    }
    printf("\n");
    strncat(buff, conduit->buffer, count);
    conduit->lecture += count;
  }
  return count;
}

void conduct_close(struct conduct * conduit){
  msync(conduit, sizeof(conduit), MS_SYNC);
  munmap(conduit, sizeof(conduit));
}

int conduct_write_eof(struct conduct *c){
  return 0;
}

void conduct_destruct(struct conduct * conduit){
  msync(conduit, sizeof(conduit), MS_SYNC);
  munmap(conduit, sizeof(conduit));
}

ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){
  if(conduit->ecriture == conduit->capacity){
    printf("Plein\n");
    return 0;
  } else {
    strncat(conduit->buffer, buff, 22);
    printf("%s\n", conduit->buffer);
    conduit->ecriture += count;
    conduct_close(conduit);
    return count;
  }
}
