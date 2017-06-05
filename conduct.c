#include "conduct.h"


struct conduct *conduct_create(const char *name, size_t a, size_t c){

    if(a > c){
        errno = EINVAL;
        return NULL;
    }

    struct conduct * conduit = NULL;

    if (name != NULL) {

        if(strlen(name) > 64){
          errno = ENAMETOOLONG;
          return NULL;
        }

        struct passwd *pw = getpwuid(getuid());
        const char *homedir = pw->pw_dir;
        char file[64];
        snprintf(file, 64, "%s/%s", homedir, name);

        if(access( file, F_OK ) != -1){
            errno = EEXIST;
            return NULL;
        }

        int fd;
        if((fd = open(file, O_CREAT | O_RDWR, 0666)) == -1){
            return NULL;
        }

        if (ftruncate(fd, sizeof(struct conduct)+c) == -1){
            return NULL;
        }

        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED){
            return NULL;
        }

        close(fd);
        strncpy(conduit->name, file, 64);

    } else {
        if ((conduit = (struct conduct *) mmap(NULL, sizeof(struct conduct)+c, PROT_WRITE | PROT_READ, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) == MAP_FAILED){
            return NULL;
        }
    }

    /*initialize the capacity*/
    conduit->capacity = c;
    conduit->atomic = a;

    /*initializing mutex*/
    pthread_mutexattr_t mutShared;
    pthread_condattr_t condShared;
    if(pthread_mutexattr_init(&mutShared) != 0){
      perror("create conduct mutexattr init");
      exit(1);
    }
    pthread_mutexattr_setpshared(&mutShared, PTHREAD_PROCESS_SHARED);
    if(pthread_condattr_init(&condShared) != 0){
      perror("create conduct condattr init");
      exit(1);
    }
    pthread_condattr_setpshared(&condShared, PTHREAD_PROCESS_SHARED);
    if(pthread_mutex_init(&conduit->mutex,&mutShared) != 0){
      perror("create conduct mutex init");
      exit(1);
    }
    if(pthread_cond_init(&conduit->cond,&condShared) != 0){
      perror("create conduct cond init");
      exit(1);
    }

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
    char file[64];
    sprintf(file, "%s/%s", homedir, name);
    if((fd = open(file, O_RDWR, 0666)) == -1){
      perror("conduct_open");
      return NULL;
    }

    if(fstat(fd, &st) == -1){
      return NULL;
    }

    if ((conduit =(struct conduct *) mmap(NULL, st.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
      return NULL;
    }

    close(fd);
    return conduit;
}

void conduct_close(struct conduct * conduit){
    if(msync(conduit, sizeof(struct conduct) + conduit->capacity, MS_SYNC) == -1){
      perror("close conduct msync");
      exit(1);
    }
    if(munmap(conduit, sizeof(struct conduct) + conduit->capacity) == -1){
      perror("close conduct munmap");
      exit(1);
    }
}

int conduct_write_eof(struct conduct *conduit){
    if(pthread_mutex_lock(&conduit->mutex) != 0){
      perror("write oef conduct lock");
      exit(1);
    }
    conduit->eof = 1;
    if(pthread_cond_broadcast(&conduit->cond) != 0){
      perror("write eof conduct broadcast");
      exit(1);
    }
    if(pthread_mutex_unlock(&conduit->mutex) != 0){
      perror("write oef conduct unlock");
      exit(1);
    }

    return 1;
}

void conduct_destroy(struct conduct * conduit){
    if(pthread_cond_destroy(&conduit->cond) != 0){
      perror("destruct conduct cond");
      exit(1);
    }

    if(pthread_mutex_destroy(&conduit->mutex) != 0){
      perror("destruct conduct mutex");
      exit(1);
    }

    if(msync(conduit, sizeof(struct conduct) + conduit->capacity, MS_SYNC) == -1){
      perror("destruct conduct msync");
      exit(1);
    }

    if(conduit->name != NULL){
      if(unlink(conduit->name) == -1){
        perror("unlink");
        exit(1);
      }
    }
    
    if(munmap(conduit, sizeof(struct conduct) + conduit->capacity) == -1){
      perror("destruct conduct munmap");
      exit(1);
    }
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

    if(pthread_mutex_lock(&conduit->mutex) != 0){
      perror("read conduct mutex lock");
      exit(1);
    }

    int lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);

    if(lect_cap == 0 && conduit->eof){
        if(pthread_mutex_unlock(&conduit->mutex) != 0){
          perror("read eof conduct mutex unlock first check");
          exit(1);
        }
        errno = EPIPE;
        return 0;
    }

    while(lect_cap == 0){
        if(pthread_cond_wait(&conduit->cond,&conduit->mutex) != 0){
          perror("read conduct cond wait");
          exit(1);
        }
        lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);
        if(lect_cap == 0 && conduit->eof){
            if(pthread_mutex_unlock(&conduit->mutex) != 0){
              perror("read eof conduct mutex unlock second check");
              exit(1);
            }
            errno = EPIPE;
            return 0;
        }
    }

    int totLect = min(lect_cap,count);
    if(conduit->loop==0 || ((conduit->lecture+totLect) <= conduit->capacity)){
        memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), totLect);
        conduit->lecture += totLect;
    } else {
        int lect1 = conduit->capacity-conduit->lecture;
        if(lect1 !=0)
	       memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), lect1);
        conduit->loop=0;
        int lect2 = totLect-lect1;

        memcpy(buff+lect1, (&(conduit->buffer_begin)), lect2);
        conduit->lecture = lect2;
    }

    if(pthread_cond_broadcast(&conduit->cond) != 0){
      perror("read conduct broadcast");
      exit(1);
    }

    if(pthread_mutex_unlock(&conduit->mutex) != 0){
      perror("read eof conduct mutex unlock");
      exit(1);
    }

    return totLect;
}

ssize_t conduct_write(struct conduct *conduit, const void* buff, size_t count){

    if(pthread_mutex_lock(&conduit->mutex) != 0){
      perror("write conduct mutex lock");
      exit(1);
    }

    if(conduit->eof){
        if(pthread_mutex_unlock(&conduit->mutex) != 0){
          perror("write eof conduct mutex unlock first time");
          exit(1);
        }
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
        if(pthread_cond_wait(&conduit->cond,&conduit->mutex) != 0){
          perror("write conduct cond wait");
          exit(1);
        }
        if(conduit->eof){
            if(pthread_mutex_unlock(&conduit->mutex) != 0){
              perror("write eof conduct mutex unlock second time");
              exit(1);
            }
            errno = EPIPE;
            return -1;
        }
        ecritureCap= conduit->capacity - lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);

        if(count >conduit->atomic)
            totEcr = min(ecritureCap,count);
        else
            totEcr = count;
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

    if(pthread_cond_broadcast(&conduit->cond) != 0){
      perror("write conduct broadcast");
      exit(1);
    }

    if(pthread_mutex_unlock(&conduit->mutex) != 0){
      perror("write conduct mutex unlock");
      exit(1);
    }

    return totEcr;
}

ssize_t try_conduct_read(struct conduct* conduit,void * buff,size_t count){
    if(pthread_mutex_trylock(&conduit->mutex) == EBUSY){
      errno = EWOULDBLOCK;
      return -1;
    }

    int lect_cap = lectCap(conduit->capacity,conduit->remplissage,conduit->lecture,conduit->loop);

    if(lect_cap == 0 && conduit->eof){
        if(pthread_mutex_unlock(&conduit->mutex) != 0){
          perror("try read eof conduct mutex unlock");
          exit(1);
        }
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
    } else {
        int lect1 = conduit->capacity-conduit->lecture;
        if(lect1 !=0)
            memcpy(buff, (&(conduit->buffer_begin)+conduit->lecture), lect1);
        conduit->loop=0;
        int lect2 = totLect-lect1;

        memcpy(buff+lect1, (&(conduit->buffer_begin)), lect2);
        conduit->lecture = lect2;
    }

    if(pthread_cond_broadcast(&conduit->cond) != 0){
      perror("try read conduct broadcast");
      exit(1);
    }

    if(pthread_mutex_unlock(&conduit->mutex) != 0){
      perror("try read conduct mutex unlock");
      exit(1);
    }

    return totLect;
}

ssize_t try_conduct_write(struct conduct *conduit, const void* buff, size_t count){
    if(pthread_mutex_trylock(&conduit->mutex) == EBUSY){
      errno = EWOULDBLOCK;
      return -1;
    }

    if(conduit->eof){
        if(pthread_mutex_unlock(&conduit->mutex) != 0){
          perror("try write eof conduct mutex unlock");
          exit(1);
        }
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
    } else {
        int ecr1 = conduit->capacity - conduit->remplissage;
        if(ecr1 !=0)
            memcpy(&(conduit->buffer_begin)+conduit->remplissage, buff,ecr1);
        int ecr2 = totEcr-ecr1;
        conduit->loop = 1;
        memcpy(&(conduit->buffer_begin),buff+ecr1,ecr2);
        conduit->remplissage =ecr2;
    }

    if(pthread_cond_broadcast(&conduit->cond) != 0){
      perror("try write conduct broadcast");
      exit(1);
    }

    if(pthread_mutex_unlock(&conduit->mutex) != 0){
      perror("try write conduct mutex unlock");
      exit(1);
    }

    return totEcr;
}

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
