#include "conduct.h"


struct conduct *conduct_create(const char *name, size_t a, size_t c){
    struct conduct * conduit = NULL;
    //    unlink(name);
    if ( name != NULL) {
        int fd_cond;
        if( access( name, F_OK ) != -1 ){
            printf("[WARNING] File already exist\n");
            return NULL;
        }
        
        if((fd_cond = open(name, O_CREAT | O_RDWR, 0666)) == -1){
            printf("1° open : %s\n", strerror(errno));
            return NULL;
        }
        
        
        if (ftruncate(fd_cond, sizeof(struct conduct)) == -1){
            printf("ftruncate failed : %s\n", strerror(errno));
            return NULL;
        }
        
        /*map the conduct with the file */
        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd_cond, 0)) ==  (void *) -1){
            printf("1° : mmap failed : %s\n", strerror(errno));
            return NULL;
        }
        
        strncpy(conduit->name, name, 15);
        close(fd_cond);
        
    } else {
        printf("creation %lu\n",sizeof(struct conduct));
        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) ==  (void *) -1){
            printf("1° : mmap failed : %s\n", strerror(errno));
            return NULL;
        }
    }
    
    /*initialize the capacity*/
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
    printf("in close\n");
    msync(conduit, sizeof(struct conduct)+conduit->capacity, MS_SYNC);
}

int conduct_write_eof(struct conduct *conduit){
    pthread_mutex_lock(&conduit->mutex);
    conduit->eof = 1;
    pthread_cond_broadcast(&conduit->cond);
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
    printf("conduit : %p,veut lire %zu, capacity %zu ,lecture : %d,ecriture %d,\n",conduit,count,conduit->capacity,conduit->lecture,conduit->remplissage);

    pthread_mutex_lock(&conduit->mutex);
    if(conduit->remplissage==conduit->lecture && conduit->eof)
        return 0;
    while(conduit->lecture >= conduit->remplissage) {
        printf("before attend%zu\n",conduit->capacity);
        printf("tranquillement %d\n",pthread_cond_wait(&conduit->cond,&conduit->mutex));
        printf("after attend%zu\n",conduit->capacity);

        if(conduit->lecture==conduit->remplissage && conduit->eof==1){
            pthread_mutex_unlock(&conduit->mutex);
            printf("stop lect\n");
            return 0;
        }
        printf("arrete attend\n");
    }
    printf("sort %d\n",conduit->capacity);
    int lect = ((conduit->remplissage-conduit->lecture < count) ? conduit->remplissage-conduit->lecture : count);
    printf("before cat %d\n",conduit->capacity);
    strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
    printf("after cat%d\n",conduit->capacity);

    conduit->lecture += lect;
    if(conduit->lecture==conduit->capacity || (conduit->lecture==conduit->remplissage && conduit->lecture > conduit->capacity- conduit->atomic)){
        printf("réduit %d\n",conduit->capacity);
        conduit->lecture = 0;
        conduit->remplissage=0;
        printf("avertit depassement\n");
        pthread_cond_broadcast(&conduit->cond);
    }
    printf("before unlock read%d\n",conduit->capacity);

    pthread_mutex_unlock(&conduit->mutex);
    printf("conduit : %p,a lu capacity %zu ,lecture : %d,ecriture %d,\n",conduit,conduit->capacity,conduit->lecture,conduit->remplissage);
    return lect;
}

ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){

    if(conduit->eof){
        errno = EPIPE;
        return -1;
    }
    pthread_mutex_lock(&conduit->mutex);
     printf("conduit : %p,veut écrire %zu, capacity %zu ,lecture : %d,ecriture %d,\n",conduit,count,conduit->capacity,conduit->lecture,conduit->remplissage);
    if(count <= conduit->atomic ){
        while(conduit->remplissage +count > conduit->capacity){
            //printf("attend ecriture %zu",conduit->capacity);
            pthread_cond_wait(&conduit->cond,&conduit->mutex);
            if(conduit->eof){
                pthread_mutex_unlock(&conduit->mutex);
                errno = EPIPE;
                //printf("stop write\n");
                return -1;
            }
        }
        memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
        conduit->remplissage += count;
    }
    else{
        if(conduit->remplissage+count > conduit->capacity){
            count = conduit->capacity-count;
        }
        strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
        conduit->remplissage += count;
    }
    pthread_mutex_unlock(&conduit->mutex);
    printf("conduit : %p a écrit capacity %zu ,lecture : %d,ecriture %d,\n",conduit,conduit->capacity,conduit->lecture,conduit->remplissage);
    pthread_cond_broadcast(&conduit->cond);
    printf("just broadcast %d",conduit->capacity);
    return count;
    
}
