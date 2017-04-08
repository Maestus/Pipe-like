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
    if( access( name, F_OK ) != -1 ) {
      char choice;
      printf("[WARNING] File already exist, Do you want to remove it ? [Y/N] : ");
      choice = getchar();
      if(choice == 'Y') {
        if(unlink(name) == -1){
          printf("Failed to delete file : %s\n", strerror(errno));
        }
      } else {
        return NULL;
      }
    }

    if((fd = open(name, O_CREAT | O_RDWR, 0666)) == -1){
        printf("open : failed in main");
        return NULL;
    }

    if (ftruncate(fd, getpagesize()+c) == -1){
      printf("ftruncate failed : %s\n", strerror(errno));
      return NULL;
    }

    if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
      printf("1° : mmap failed : %s\n", strerror(errno));
      return NULL;
    }

    conduit->is_anon = 0;

    conduit->name = malloc(15*sizeof(char));
    strcpy(conduit->name, name);
    conduit->capacity = c;
    if((conduit->buffer = (char *) mmap(NULL, conduit->capacity*sizeof(char), PROT_WRITE | PROT_READ, MAP_SHARED, fd, getpagesize())) == (void *) -1){
      printf("2° : mmap failed : %s\n", strerror(errno));
      return NULL;
    }
  } else {

    if((fd = open(generate_name(), O_CREAT | O_RDWR, 0666)) == -1){
        printf("shm_open : failed in main");
        return NULL;
    }

    if (ftruncate(fd, getpagesize()+c) == -1){
      printf("ftruncate failed : %s\n", strerror(errno));
      return NULL;
    }

    if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
      printf("mmap failed : %s\n", strerror(errno));
      return NULL;
    }

    conduit->is_anon = 1;

    conduit->name = malloc(15*sizeof(char));
    strcpy(conduit->name, name);
    conduit->capacity = c;
    if((conduit->buffer = (char *) mmap(NULL, conduit->capacity*sizeof(char), PROT_WRITE | PROT_READ, MAP_SHARED, fd, getpagesize())) == (void *) -1){
      printf("2° : mmap failed : %s\n", strerror(errno));
      return NULL;
    }
  }
  memset(conduit->buffer, 0, conduit->capacity);
  conduit->atomic = a;
  conduit->lecture = 0;
  conduit->ecriture = 0;
  conduit->remplissage = 0;
  close(fd);
  return conduit;
}

struct conduct *conduct_open(const char *name){
  int fd;
  struct conduct * conduit;

  if((fd = open(name, O_RDWR)) == -1){
      printf("shm_open : failed in main");
      return NULL;
  }

  if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
    printf("mmap failed : %s\n", strerror(errno));
    return NULL;
  }

  if((conduit->buffer = (char *) mmap(NULL, conduit->capacity*sizeof(char), PROT_WRITE | PROT_READ, MAP_SHARED, fd, getpagesize())) == (void *) -1){
    printf("2° : mmap failed : %s\n", strerror(errno));
    return NULL;
  }

  printf("%s\n", conduit->buffer);
  close(fd);
  return conduit;
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



ssize_t conduct_read(struct conduct * conduit, void * buff, size_t count){
  while(conduit->lecture == conduit->ecriture) {}
  if(conduit->lecture < conduit->capacity){
    printf("[%s|%zu]\n", conduit->buffer, conduit->capacity);
    for (size_t i = conduit->lecture; i < conduit->lecture+count; i++) {
      printf("%c", conduit->buffer[i]);
    }
    printf("\n");
    strncat(buff, conduit->buffer, count);
    conduit->lecture += count;
    printf("%d : end\n", conduit->lecture);
  }
  return count;
}

ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){
  if(conduit->ecriture == conduit->capacity){
    printf("Plein\n");
    return 0;
  } else {
    strncpy(conduit->buffer, buff, 22);
    conduit->ecriture += count;
    return count;
  }
}
