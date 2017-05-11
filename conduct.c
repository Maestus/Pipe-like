#include "conduct.h"


struct conduct *conduct_create(const char *name, size_t a, size_t c){

    if(a > c){
        errno = EINVAL;
        return NULL;
    }

    struct conduct * conduit = NULL;
    if ( name != NULL) {
        if(strlen(name) > 64){
            errno = ENAMETOOLONG;
            return NULL;
        }
        struct passwd *pw = getpwuid(getuid());
        const char *homedir = pw->pw_dir;
        printf("%s\n", homedir);
        char file[128];
        sprintf(file, "%s/%s", homedir, name);
        int fd;
        if( access( file, F_OK ) != -1 ){
            errno = EEXIST;
            return NULL;
        }

        if((fd = open(file, O_CREAT | O_RDWR, 0666)) == -1){
            return NULL;
        }

        if (ftruncate(fd, sizeof(struct conduct)+c) == -1){
            return NULL;
        }

        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
            return NULL;
        }

        strncpy(conduit->name, name, 64);

    } else {
        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) ==  (void *) -1){
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
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    char file[128];
    sprintf(file, "%s/%s", homedir, name);
    if((fd = open(file, O_RDWR, 0666)) == -1){
        return NULL;
    }

    if(fstat(fd, &st) == -1){
        return NULL;
    }

    if ((conduit =(struct conduct *) mmap(NULL, st.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
        return NULL;
    }

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

void conduct_destroy(struct conduct * conduit){
    if(pthread_cond_destroy(&conduit->cond)>=0)
        printf("succes\n");
    else
        printf("unsuccessfull\n");

    if(pthread_mutex_destroy(&conduit->mutex)>=0)
        printf("succes\n");
    else
        printf("unsuccessfull\n");
    unlink(conduit->name);
    msync(conduit, sizeof(conduit), MS_SYNC);
    munmap(conduit, sizeof(conduit));

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
    //printf("avant le lock\n");
    pthread_mutex_lock(&conduit->mutex);
    //printf("prise du lock\n");
    int lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);

    if(lect_cap == 0 && conduit->eof){
        pthread_mutex_unlock(&conduit->mutex);
        errno = EPIPE;
        return 0;
    }

    while(lect_cap == 0){
        //printf("en attente\n");
        pthread_cond_wait(&conduit->cond,&conduit->mutex);
        //printf("sortit de l'attente\n");
        lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
        if(lect_cap == 0 && conduit->eof){
            pthread_mutex_unlock(&conduit->mutex);
            errno = EPIPE;
            return 0;
        }
        //printf("new cap\n");
    }
    //printf("sortit du while\n");
    int totLect = min(lect_cap,count);
    if(conduit->loop==0 || ((conduit->lecture+totLect) <= conduit->capacity)){
        memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), totLect);
        conduit->lecture += totLect;
    }
    else{
        int lect1 = conduit->capacity-conduit->lecture;
        if(lect1 !=0)
	       memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), lect1);
        conduit->loop=0;
        int lect2 = totLect-lect1;

        memcpy(buff+lect1, (&(conduit->buffer_begin)), lect2);
        conduit->lecture = lect2;
    }
    //printf("broadcat\n");
    pthread_cond_broadcast(&conduit->cond);
    pthread_mutex_unlock(&conduit->mutex);
    //printf("unlock\n");
    return totLect;
}

ssize_t conduct_write(struct conduct *conduit, const void* buff, size_t count){
    //printf("avant le lock2\n");
    pthread_mutex_lock(&conduit->mutex);

    /****************************************************
    Besoin de trouver un moyen de voir si 'count' et
    superieur à la taille du buffer 'buff'
    errno = ENOMEM ?
    ****************************************************/

    if(conduit->eof){
        pthread_mutex_unlock(&conduit->mutex);
        errno = EPIPE;
        return -1;
    }
    int ecritureCap= conduit->capacity - lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
    int totEcr;
    if(count >conduit->atomic)
        totEcr = min(ecritureCap,count);
    else
        totEcr = count;
    while(ecritureCap==0 || (totEcr > ecritureCap)){
        //printf("wait2\n");
        pthread_cond_wait(&conduit->cond,&conduit->mutex);
        if(conduit->eof){
            //printf("eof\n");
            pthread_mutex_unlock(&conduit->mutex);
            errno = EPIPE;
            return -1;
        }
        ecritureCap= conduit->capacity - lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);

        if(count >conduit->atomic)
            totEcr = min(ecritureCap,count);
        else
            totEcr = count;
    }
    //printf("sortit du while2\n");
    if((totEcr+conduit->remplissage) <= conduit->capacity){
        memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff,totEcr);
        conduit->remplissage +=totEcr;
    }
    else{
        int ecr1 = conduit->capacity - conduit->remplissage;
        if(ecr1 !=0)
            memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff,ecr1);
        int ecr2 = totEcr-ecr1;
        conduit->loop = 1;
        memcpy(&(conduit->buffer_begin),buff+ecr1,ecr2);
        conduit->remplissage =ecr2;
    }
    //printf("broadcast2\n");

    pthread_cond_broadcast(&conduit->cond);

    pthread_mutex_unlock(&conduit->mutex);
    //printf("unlock2\n");

    return totEcr;
}

ssize_t try_conduct_read(struct conduct* conduit,void * buff,size_t count){
    if(pthread_mutex_trylock(&conduit->mutex)!=EBUSY){

    int lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);

    if(lect_cap == 0 && conduit->eof){
        pthread_mutex_unlock(&conduit->mutex);
        errno = EPIPE;
        return 0;
    }

    if(lect_cap == 0){
        errno = EWOULDBLOCK;
        return 0;
    }

    int totLect = min(lect_cap,count);
    if(conduit->loop==0 || ((conduit->lecture+totLect) <= conduit->capacity)){
        memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), totLect);
        conduit->lecture += totLect;
    }
    else{
        int lect1 = conduit->capacity-conduit->lecture;
        if(lect1 !=0)
            memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), lect1);
        conduit->loop=0;
        int lect2 = totLect-lect1;

        memcpy(buff+lect1, (&(conduit->buffer_begin)), lect2);
        conduit->lecture = lect2;
    }
    pthread_cond_broadcast(&conduit->cond);
    pthread_mutex_unlock(&conduit->mutex);
    return totLect;
    }
    else
        errno = EWOULDBLOCK;
        return -1;
}

ssize_t try_conduct_write(struct conduct *conduit, const void* buff, size_t count){
    if(pthread_mutex_trylock(&conduit->mutex)!=EBUSY){


    if(conduit->eof){
        pthread_mutex_unlock(&conduit->mutex);
        errno = EPIPE;
        return -1;
    }
    int ecritureCap= conduit->capacity - lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
    int totEcr;
    if(count >conduit->atomic)
        totEcr = min(ecritureCap,count);
    else
        totEcr = count;
    if(ecritureCap==0 || (totEcr > ecritureCap)){
        errno = EWOULDBLOCK;
        return -1;
    }
    if((totEcr+conduit->remplissage) <= conduit->capacity){
        memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff,totEcr);
        conduit->remplissage +=totEcr;
    }
    else{
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
    else
        errno = EWOULDBLOCK;
        return -1;
}
/*int stillExist(struct conduct *conduit){
    if(conduit==NULL)
        return 0;
    else if(name != NULL && access(name,F_OK))
        return 0;
    else
        return -1;

}*/

ssize_t conduct_writev(struct conduct *conduit, const struct iovec *iov, int iovcnt){
    size_t bytes = 0;
    for (int i = 0; i < iovcnt; ++i) {
        bytes += iov[i].iov_len;
    }

    size_t to_copy = bytes;
    for (size_t i = 0; i < iovcnt; i++) {
        for (size_t j = 0; j < iov[i].iov_len; j++) {
            conduct_write(conduit, iov[i].iov_base+j, 1);
            to_copy -= 1;
        }
    }
    return bytes;
}

ssize_t conduct_readv(struct conduct *conduit, const struct iovec *iov, int iovcnt){
    size_t bytes = 0;
    for (int i = 0; i < iovcnt; ++i) {
        bytes += iov[i].iov_len;
    }


    size_t to_read = bytes;
    for (size_t i = 0; i < iovcnt; i++) {
        for (size_t j = 0; j < iov[i].iov_len; j++) {
            conduct_read(conduit, iov[i].iov_base+j, 1);
            to_read -= 1;
        }
    }
    return bytes;
}
