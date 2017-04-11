#include "conduct.h"

struct conduct *conduct_create(const char *name, size_t a, size_t c){
  int fd_cond;
  struct conduct * conduit = NULL;

  if ( name != NULL) {
    if( access( name, F_OK ) != -1 ){
        printf("[WARNING] File already exist\n");
        return NULL;
    }

    if((fd_cond = open(name, O_CREAT | O_RDWR, 0666)) == -1){
      printf("1° open : failed in main");
      return NULL;
    }


    if (ftruncate(fd_cond, sizeof(struct conduct)+c) == -1){
      printf("ftruncate failed : %s\n", strerror(errno));
      return NULL;
    }

    if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED, fd_cond, 0)) ==  (void *) -1){
      printf("1° : mmap failed : %s\n", strerror(errno));
      return NULL;
    }

    conduit->buffer_begin = sizeof(struct conduct) + 1;
    strncpy(conduit->name, name, 15);
    conduit->capacity = c;


  } else {
    /*

    A faire

    */
  }
  conduit->atomic = a;
  conduit->lecture = 0;
  conduit->remplissage = 0;

  return conduit;
}

struct conduct *conduct_open(const char *name){
  int fd;
  struct conduct * conduit;

  if((fd = open(name, O_RDWR,0666)) == -1){
      printf("open file : failed in main");
      return NULL;
  }

  if ((conduit =(struct conduct *) mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  MAP_FAILED){
    printf("mmap failed : %s\n", strerror(errno));
    return NULL;
  }

  int capacity = conduit->capacity;

  printf("=%d\n", conduit->remplissage);


  if(munmap(conduit, sizeof(struct conduct)) == -1){
    printf("munmap failed : %s\n", strerror(errno));
    return NULL;
  }

  if ((conduit =(struct conduct *) mmap(NULL, sizeof(struct conduct)+capacity, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  MAP_FAILED){
    printf("mmap failed : %s\n", strerror(errno));
    return NULL;
  }

  printf("=%d\n", conduit->remplissage);

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
  int lect = ((conduit->remplissage < (int)count) ? conduit->remplissage : (int)count);
  //printf("%d ~~~~ %zu\n", conduit->remplissage, count);
  //printf("~~~ %d ~~~\n", lect);
  /*for (size_t i = 0; i < lect; i++) {
    printf("[%c]", (&(conduit->buffer_begin)+conduit->lecture)[i]);
  }
  printf("\n");*/
  strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
  conduit->lecture += lect;
  conduit->remplissage -= lect;
  //printf("%d : end\n", conduit->lecture);
  return count;
}

ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){
  if(conduit->remplissage == conduit->capacity){
    printf("Plein\n");
    return 0;
  } else {
    memcpy(&(conduit->buffer_begin), buff, count);
    conduit->remplissage += count;
    return count;
  }
}
