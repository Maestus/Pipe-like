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
    printf("Le remplissage : %s\n", conduit->name);

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

ssize_t conduct_read(struct conduct * conduit, void * buff, size_t count){

    pthread_mutex_lock(&conduit->mutex);
    if(conduit->eof)
        return 0;

    int lect;

    while(conduit->lecture == conduit->remplissage) {
        printf("Attente...\n");
        pthread_cond_wait(&conduit->cond,&conduit->mutex);
        if(conduit->eof){
            pthread_mutex_unlock(&conduit->mutex);
            errno = EPIPE;
            return -1;
        }
    }

    if((conduit->lecture+count)%conduit->capacity <= conduit->remplissage){
        if (conduit->remplissage > conduit->lecture){
            lect = (conduit->lecture+count)%conduit->capacity;
            strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
            conduit->lecture += lect;
        } else {
            lect = conduit->capacity - conduit->lecture;
            strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
            int lecture2 = count - lect;
            conduit->lecture = 0;
            strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lecture2);
            conduit->lecture = lecture2;
        }
    } else {
        if (conduit->remplissage > conduit->lecture){
            lect = conduit->remplissage-conduit->lecture;
            strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
            conduit->lecture += lect;
        } else {
            lect = conduit->capacity - conduit->lecture;
            strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
            conduit->lecture = 0;
            lect = conduit->remplissage;
            strncat(buff, (&(conduit->buffer_begin)+conduit->lecture), lect);
            conduit->lecture = lect;
        }
    }

    pthread_cond_broadcast(&conduit->cond);
    pthread_mutex_unlock(&conduit->mutex);
    return lect;
}


ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){
    pthread_mutex_lock(&conduit->mutex);
    if(conduit->eof){
        errno = EPIPE;
        return -1;
    }
    int ecriture = 0;

    if(conduit->remplissage < conduit->lecture){ // on est avant la tete de lecture
        if ((conduit->remplissage+count) < (conduit->lecture)){ //ecriture cyclique
            strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
            conduit->remplissage += count;
            ecriture = count;
        } else {
            if((count <= conduit->atomic)){
                while((conduit->remplissage + count)%conduit->capacity >= conduit->lecture){
                    printf("attend ecriture %zu",conduit->capacity);
                    pthread_cond_wait(&conduit->cond,&conduit->mutex);
                    if(conduit->eof){
                        pthread_mutex_unlock(&conduit->mutex);
                        errno = EPIPE;
                        return -1;
                    }
                }
                strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
                conduit->remplissage += count;
                ecriture = count;
            } else {
                strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, conduit->lecture-1);
                conduit->remplissage += conduit->lecture-1;
                ecriture = conduit->lecture-1;
            }
        }

    } else if(conduit->remplissage > conduit->lecture) { // on est apres la tete de lecture
        if ((conduit->remplissage+count)%conduit->capacity < (conduit->lecture)){ //ecriture ne depasse pas
            if(conduit->remplissage+count > conduit->capacity){ // ecrire en fin puis au debut du tube
                int part_1 = conduit->capacity-conduit->remplissage;
                strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, part_1);
                conduit->remplissage = 0;
                int part_2 = count - part_1;
                strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, part_2);
                conduit->remplissage = part_2;
                ecriture = part_1 + part_2;
            } else { // ecrire au debut
                strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
                conduit->remplissage += count;
                ecriture = count;
            }
        } else { // ecriture depasse
            if((count <= conduit->atomic)){
                while((conduit->remplissage + count)%conduit->capacity >= conduit->lecture){
                    printf("attend ecriture %zu",conduit->capacity);
                    pthread_cond_wait(&conduit->cond,&conduit->mutex);
                    if(conduit->eof){
                        pthread_mutex_unlock(&conduit->mutex);
                        errno = EPIPE;
                        return -1;
                    }
                }
                strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, count);
                conduit->remplissage += count;
                ecriture = count;
            } else {
                int part_1 = conduit->capacity-conduit->remplissage;
                strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, part_1);
                conduit->remplissage = 0;
                int part_2 = count - part_1;
                if(part_2 >= conduit->lecture){
                    int partition = conduit->lecture - 1;
                    strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, partition);
                    conduit->remplissage = partition;
                    ecriture = part_1 + partition;
                } else {
                    strncpy(&(conduit->buffer_begin)+conduit->remplissage, buff, part_2);
                    conduit->remplissage = part_2;
                    ecriture = part_1 + part_2;
                }
            }
        }
    }

    pthread_cond_broadcast(&conduit->cond);
    pthread_mutex_unlock(&conduit->mutex);
    return ecriture;
}
