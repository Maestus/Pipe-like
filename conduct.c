#include "conduct.h"


struct conduct *conduct_create(const char *name, size_t a, size_t c){
    struct conduct * conduit = NULL;
    unlink(name);
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
        
        /*map the conduct with the file */
        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED, fd_cond, 0)) ==  (void *) -1){
            printf("1° : mmap failed : %s\n", strerror(errno));
            return NULL;
        }
        
        strncpy(conduit->name, name, 15);
        conduit->capacity = c;
        
        /*initializing mutex*/
        pthread_mutexattr_t mutShared;
        pthread_condattr_t condShared;
        pthread_mutexattr_init(&mutShared);
        pthread_mutexattr_setpshared(&mutShared,PTHREAD_PROCESS_SHARED);
        pthread_condattr_init(&condShared);
        pthread_condattr_setpshared(&condShared,
                                    PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&conduit->mutex,&mutShared);
        pthread_cond_init(&conduit->cond,&condShared);
        
        /*intializing offset*/
        conduit->atomic = a;
        conduit->lecture = 0;
        conduit->remplissage = 0;
        conduit->eof = 0;
        conduit->buffer_begin = sizeof(struct conduct) + 1;
        
        close(fd_cond);
        
    } else {
        if((conduit = (struct conduct *) mmap(NULL,sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) ==  (void *) -1){
            printf("1° : mmap failed : %s\n", strerror(errno));
            return NULL;
        }
    }
    /*initializing mutex*/
    pthread_mutexattr_t mutShared;
    pthread_condattr_t condShared;
    pthread_mutexattr_init(&mutShared);
    pthread_mutexattr_setpshared(&mutShared,PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&condShared);
    pthread_condattr_setpshared(&condShared,
                                PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&conduit->mutex,&mutShared);
    pthread_cond_init(&conduit->cond,&condShared);
    
    /*intializing offset*/
    conduit->atomic = a;
    conduit->lecture = 0;
    conduit->remplissage = 0;
    conduit->eof = 0;
    conduit->buffer_begin = sizeof(struct conduct) + 1;
    
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

int conduct_write_eof(struct conduct *conduit){
    pthread_mutex_lock(&conduit->mutex);
    conduit->eof = 1;
    pthread_mutex_unlock(&conduit->mutex);

    return 1;
}

void conduct_destruct(struct conduct * conduit){
    msync(conduit, sizeof(conduit), MS_SYNC);
    munmap(conduit, sizeof(conduit));
    pthread_mutex_destroy(&conduit->mutex);
    unlink(conduit->name);

}

ssize_t conduct_read(struct conduct * conduit, void * buff, size_t count){
    
    pthread_mutex_lock(&conduit->mutex);
    if(conduit->remplissage==conduit->lecture && conduit->eof)
        return 0;
    while(conduit->lecture >= conduit->remplissage) {printf("attend\n");pthread_cond_wait(&conduit->cond,&conduit->mutex);}
    
    int lect = ((conduit->remplissage-conduit->lecture < count) ? conduit->remplissage-conduit->lecture : count);
    strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
    conduit->lecture += lect;
    if(conduit->lecture==conduit->capacity || (conduit->lecture==conduit->remplissage && conduit->lecture > conduit->capacity- conduit->atomic)){
        conduit->lecture = 0;
        conduit->remplissage=0;
        printf("avertit depassement\n");
        pthread_cond_broadcast(&conduit->cond);
    }
    pthread_mutex_unlock(&conduit->mutex);
    return lect;
}

ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){
    if(conduit->eof){
        errno = EPIPE;
        return -1;
    }
    pthread_mutex_lock(&conduit->mutex);
    if(count < conduit->atomic ){
        while(conduit->remplissage +count > conduit->capacity){
            printf("attend ecriture %zu",conduit->capacity);
            pthread_cond_wait(&conduit->cond,&conduit->mutex);
        }
        memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
        conduit->remplissage += count;
        pthread_cond_broadcast(&conduit->cond);
    }
    else{
        if(conduit->remplissage+count > conduit->capacity){
            count = conduit->capacity-count;
        }
        memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
        conduit->remplissage += count;
    }
    pthread_mutex_unlock(&conduit->mutex);
    return count;

}
