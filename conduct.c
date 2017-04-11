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
        close(fd_cond);

        conduit->buffer_begin = sizeof(struct conduct) + 1;
        strncpy(conduit->name, name, 15);
        conduit->capacity = c;
        
    } else {
        /*
         
         A faire
         
         */
    }
    pthread_mutex_init(&((&conduit->mutex)),NULL);
    conduit->atomic = a;
    conduit->lecture = 0;
    conduit->ecriture = 0;
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
    
    struct stat fileInfo = {0};
    
    if (fstat(fd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }
    
    if (fileInfo.st_size == 0)
    {
        fprintf(stderr, "Error: File is empty, nothing to do\n");
        exit(EXIT_FAILURE);
    }
    
    if ((conduit =(struct conduct *) mmap(NULL, sizeof(struct conduct), PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0)) ==  (void *) -1){
        printf("mmap failed : %s\n", strerror(errno));
        return NULL;
    }
    
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
    pthread_mutex_destroy((&conduit->mutex));
}



ssize_t conduct_read(struct conduct * conduit, void * buff, size_t count){
    pthread_mutex_lock((&conduit->mutex));
    while(conduit->lecture == conduit->ecriture) {pthread_cond_wait((&conduit->cond),(&conduit->mutex));}
    if(conduit->lecture < conduit->capacity){
        for (size_t i = conduit->lecture; i < count; i++) {
            printf("%c",  (&(conduit->buffer_begin)+conduit->lecture)[i]);
        }
        printf("\n");
        /*strncat(buff, (char *) (conduit->buffer_begin+conduit->lecture), count);*/
        conduit->lecture += count;
        printf("%d : end\n", conduit->lecture);
    }
    pthread_mutex_unlock((&conduit->mutex));
    return count;
}

ssize_t conduct_write(struct conduct * conduit, const void * buff, size_t count){
    if(conduit->ecriture == conduit->capacity){
        printf("Plein\n");
        return 0;
    } else {
        pthread_mutex_lock((&conduit->mutex));
        strncpy((&conduit->buffer_begin)+conduit->ecriture, buff, count);
        conduit->ecriture += count;
        pthread_mutex_unlock((&conduit->mutex));
        pthread_cond_signal((&conduit->cond));
        return count;
    }

}
