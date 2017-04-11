#include "conduct.h"


struct conduct *conduct_create(const char *name, size_t a, size_t c){
    struct conduct * conduit = NULL;
    
    if ( name != NULL) {
        int fd_cond;
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
        
        strncpy(conduit->name, name, 15);
        conduit->capacity = c;
        pthread_mutex_init(&conduit->mutex,NULL);
        conduit->atomic = a;
        conduit->lecture = 0;
        conduit->remplissage = 0;
        conduit->buffer_begin = sizeof(struct conduct) + 1;
        
        close(fd_cond);
        
    } else {
        /*
         
         A faire
         
         */
    }
    return conduit;
}

struct conduct *conduct_open(const char *name){
    int fd;
    struct conduct * conduit;
    
    if((fd = open(name, O_RDWR,0666)) == -1){
        printf("open file : failed in main");
        return NULL;
    }
    
    if ((conduit =(struct conduct *) mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
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
    pthread_mutex_destroy(&conduit->mutex);
}

ssize_t conduct_read(struct conduct * conduit, void * buff, size_t count){
    
    pthread_mutex_lock(&conduit->mutex);
    while(conduit->lecture > conduit->remplissage) {pthread_cond_wait(&conduit->cond,&conduit->mutex);}
    
    int lect = ((conduit->remplissage < (int)count) ? conduit->remplissage : (int)count);
    strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
    conduit->lecture += lect;
    conduit->remplissage -= lect;
    
    
    pthread_mutex_unlock(&conduit->mutex);
    return count;
}

ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){
    if(conduit->remplissage == conduit->capacity){
        printf("Plein\n");
        return 0;
    } else {
        pthread_mutex_lock(&conduit->mutex);
        memcpy(&(conduit->buffer_begin), buff, count);
        conduit->remplissage += count;
        conduit->lecture -= count;
        pthread_mutex_unlock(&conduit->mutex);
        pthread_cond_signal(&conduit->cond);
        return count;
    }
}
