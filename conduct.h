#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/uio.h>
#include <pwd.h>

/** \struct conduct
 *\brief structure permettant à différent processus de communiquer en mémoire partagé
 */

struct conduct{
    char name[64];
    size_t capacity;
    size_t atomic;
    int remplissage;
    int lecture;
    int eof;
    int loop;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    char buffer_begin;
};

/** \fn struct conduct *conduct_create (const char *name, size_t a, size_t c)
 *\brief fonction qui crée un conduit 
 *\param name chaine de charactère contenant le nom du conduit, null si c'est un conduit anonyme
 *\param a taille du conduit
 *\param c atomicité du conduit doit être inférieur à a
 *\return retourne le conduit crée
 */
struct conduct *conduct_create(const char *name, size_t a, size_t c);


/** \fn struct conduct *conduct_open(const char *name)
 *\brief fonction qui ouvre un conduit nommé existant
 *\param name chaine de caractère contenant le nom du conduit que l'on veut ouvrir
 *\return le conduit de nom name, null si inexistant
 */
struct conduct *conduct_open(const char *name);

/** \fn ssize_t conduct_read(struct conduct *c, void *buf, size_t count)
 *\brief fonction qui lit dans un conduit, bloquant en dessous de l'atomicité sinon renvoie lit le maximum d'octet possible
 *\param c le conduit dans lequel on lit
 *\param buf le buffer dans lequel on stoque la lecture
 *\count le nombre d'octet qu'on souhaite lire
 *\return le nombre d'octet lu
 */


ssize_t conduct_read(struct conduct *c, void *buf, size_t count);

/** \fn ssize_t conduct_write(struct conduct *c, void *buf, size_t count)
 *\brief fonction qui écrit dans un conduit, bloquant en dessous de l'atomicité sinon écrit le maximum d'octet possible
 *\param c le conduit dans lequel on écrit
 *\param buf le buffer qui contient ce que l'on veut écrire
 *\count le nombre d'octet qu'on souhaite écrire
 *\return le nombre d'octet écrit
 */
ssize_t conduct_write(struct conduct *c, const void *buf, size_t count);

/** \fn ssize_t try_conduct_read(struct conduct *c, void *buf, size_t count)
 *\brief fonction qui lit dans un conduit, si il n'y a pas la place échou en dessous de l'atomicité sinon lit le maximum possible
 *\param c le conduit dans lequel on lit
 *\param buf le buffer dans lequel on stoque la lecture
 *\count le nombre d'octet qu'on souhaite lire
 *\return le nombre d'octet lu
 */
ssize_t try_conduct_read(struct conduct* conduit,void * buff,size_t count);

/** \fn ssize_t try_conduct_write(struct conduct *c, void *buf, size_t count)
 *\brief fonction qui écrit dans un conduit, si il n'y a pas la place échou en dessous de l'atomicité sinon écrit le maximum possible
 *\param c le conduit dans lequel on écrit
 *\param buf le buffer qui contient ce que l'on veut écrire
 *\count le nombre d'octet qu'on souhaite écrire
 *\return le nombre d'octet écrit
 */
ssize_t try_conduct_write(struct conduct *conduit, const void* buff, size_t count);

/** \fn int conduct_write_eof(struct conduct *c)
 *\brief interdit l'écriture dans un fichier
 *\param c le conduit dans lequel on écrit le caractère de fin de fichier
 *\return 1
 */
int conduct_write_eof(struct conduct *c);

/** \fn void conduct_close(struct conduct *conduct)
 *\brief ferme un conduit
 *\param c le conduit que l'on souhaite fermé
 *\return void
 */
void conduct_close(struct conduct *conduct);

/** \fn void conduct_close(struct conduct *conduct)
 *\brief détruit un conduit, attention à ce que le conduit ne soit pas utilisé par d'autre processus
 *\param c le conduit que l'on souhaite détruire
 *\return void
 */
void conduct_destroy(struct conduct *conduct);
ssize_t conduct_readv(struct conduct *conduit, const struct iovec *iov, int iovcnt);
ssize_t conduct_writev(struct conduct *conduit, const struct iovec *iov, int iovcnt);
