#include "conduct.h"


struct conduct *conduct_create(const char *name, size_t a, size_t c){
    struct conduct * conduit = NULL;
    //    unlink(name);
    if ( name != NULL) {
        int fd;
        if( access( name, F_OK ) != -1 ){
            printf("[WARNING] File already exist\n");
            return NULL;
        }

        if((fd = open(name, O_CREAT | O_RDWR, 0666)) == -1){
            printf("1° open : %s\n", strerror(errno));
            return NULL;
        }


        if (ftruncate(fd, sizeof(struct conduct)+c) == -1){
            printf("ftruncate failed : %s\n", strerror(errno));
            return NULL;
        }

        /*map the conduct with the file */
        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
            printf("1° : mmap failed : %s\n", strerror(errno));
            return NULL;
        }

        strncpy(conduit->name, name, 64);

    } else {
        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) ==  (void *) -1){
            printf("1° : mmap failed : %s\n", strerror(errno));
            return NULL;
        }
    }

    /*initialize the capacity*/
    conduit->capacity = c;
    conduit->atomic = a;

    /*initializing mutex*/
    pthread_mutexattr_t mutShared;
    pthread_condattr_t condShared;
    pthread_mutexattr_init(&mutShared);
    pthread_mutexattr_setpshared(&mutShared, PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&condShared);
    pthread_condattr_setpshared(&condShared, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&conduit->mutex,&mutShared);
    pthread_cond_init(&conduit->cond,&condShared);

    /*intializing offset*/
    conduit->lecture = 0;
    conduit->remplissage = 0;
    conduit->eof = 0;
    conduit->loop = 0;

    return conduit;
}

struct conduct *conduct_open(const char *name){
    int fd;
    struct conduct * conduit;
    struct stat st;

    if((fd = open(name, O_RDWR, 0666)) == -1){
        printf("open file : failed in main");
        return NULL;
    }

    fstat(fd, &st);

    if ((conduit =(struct conduct *) mmap(NULL, st.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
        printf("mmap failed : %s\n", strerror(errno));

        return NULL;
    }

    printf("Le remplissage : %d\n", conduit->remplissage);
    printf("La lecture : %d\n", conduit->lecture);

    return conduit;
}

void conduct_close(struct conduct * conduit){
    msync(conduit, sizeof(conduit), MS_SYNC);
    munmap(conduit, sizeof(conduit));
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

int lectCap(int capacity,int posEcr,int posLect,int loop){
    int lect_cap;
    if(loop==0){
        lect_cap = posEcr-posLect;
    }
    else{
        lect_cap = capacity-posLect+posEcr;
    }

    return lect_cap;
}

int min(int a,int b){
    if(a<b)
        return a;
    else return b;
}
ssize_t conduct_read(struct conduct* conduit,void * buff,size_t count){
    pthread_mutex_lock(&conduit->mutex);
    if(conduit->eof)
        return 0;
    int lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
    // printf("lecture Cap %d,  %d\n",lect_cap,conduit->capacity);

    while(lect_cap ==0){
        pthread_cond_wait(&conduit->cond,&conduit->mutex);
        if(conduit->eof){
            printf("eof");
            pthread_mutex_unlock(&conduit->mutex);
            errno = EPIPE;
            return -1;
        }
        lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
        // printf("lecture Cap %d ,%d\n ",lect_cap,conduit->capacity);

    }
    int totLect = min(lect_cap,count);
    printf("In read : %d %d %d %ld %d\n", conduit->remplissage, conduit->lecture, lect_cap, count, totLect);
    if(conduit->loop==0 || ((conduit->lecture+totLect) <= conduit->capacity)){
        printf("vais lire %d\n", totLect);
        memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), totLect);
        conduit->lecture += totLect;
    }
    else{
        int lect1 = conduit->capacity-conduit->lecture;

        memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), lect1);
        conduit->loop=0;
        int lect2 = totLect-lect1;

        memcpy(buff, (&(conduit->buffer_begin)), lect2);
        conduit->lecture = lect2;
    }
    pthread_cond_broadcast(&conduit->cond);
    printf("end\n");
    pthread_mutex_unlock(&conduit->mutex);
    return totLect;
}

ssize_t conduct_write(struct conduct *conduit,const void* buff,size_t count){
    pthread_mutex_lock(&conduit->mutex);
    if(conduit->eof){
        pthread_mutex_unlock(&conduit->mutex);
        errno = EPIPE;
        return -1;
    }
    int ecritureCap= conduit->capacity - lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
    //printf("ecriture Cap %d,  %d\n",ecritureCap,conduit->capacity);
    int totEcr;
    if(count >conduit->atomic)
        totEcr = min(ecritureCap,count);
    else
        totEcr = count;
    while(ecritureCap==0 || (totEcr > ecritureCap)){
        pthread_cond_wait(&conduit->cond,&conduit->mutex);
        if(conduit->eof){
            printf("eof\n");
            pthread_mutex_unlock(&conduit->mutex);
            errno = EPIPE;
            return -1;
        }
        ecritureCap= conduit->capacity - lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
         printf("ecriture Cap %d,  %ld\n",ecritureCap,conduit->capacity);

        if(count >conduit->atomic)
            totEcr = min(ecritureCap,count);
        else
            totEcr = count;
    }
    if((totEcr+conduit->remplissage) <= conduit->capacity){
        memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff,totEcr);
        conduit->remplissage +=totEcr;
        // printf("totEcr %d\n",totEcr);
    }
    else{
        printf("depassement\n");
        int ecr1 = conduit->capacity - conduit->remplissage;
        if(ecr1 !=0)
            memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff,ecr1);
        int ecr2 = totEcr-ecr1;
        conduit->loop = 1;
        memcpy(&(conduit->buffer_begin),buff+ecr1,ecr2);
        conduit->remplissage =ecr2;
    }
    pthread_cond_broadcast(&conduit->cond);
    pthread_mutex_unlock(&conduit->mutex);
    return totEcr;
}
