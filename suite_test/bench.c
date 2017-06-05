#include "../conduct.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

struct timespec start, stop;

struct conduct * conduit;

int com[2];

#define SOCK_PATH "/tmp/tpf_unix_sock.server"
int server_socket;
int client_socket;
struct sockaddr_un server_sockaddr, peer_sock;
struct sockaddr_un remote;

void server_exchange(char * empty, int size){
  socklen_t len = sizeof(server_sockaddr);
  if(recvfrom(server_socket, empty, size, 0, (struct sockaddr *) &peer_sock, &len) == -1){
    perror("server recvfrom");
    close(server_socket);
    exit(1);
  }
}


void client_exchange(char * buffer, char * empty, int size){
  if(sendto(client_socket, buffer, size, 0, (struct sockaddr *) &remote, sizeof(remote)) == -1){
    perror("sendto");
    close(client_socket);
    exit(1);
  }
}


void start_server(){
  int len, rc;
  memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
  server_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
  if (server_socket == -1){
    perror("socket");
    exit(1);
  }

  server_sockaddr.sun_family = AF_UNIX;
  strcpy(server_sockaddr.sun_path, SOCK_PATH);
  len = sizeof(server_sockaddr);
  unlink(SOCK_PATH);
  rc = bind(server_socket, (struct sockaddr *) &server_sockaddr, len);
  if (rc == -1){
    perror("bind");
    close(server_socket);
    exit(1);
  }
}


void start_client(void){
    memset(&remote, 0, sizeof(struct sockaddr_un));
    client_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (client_socket == -1) {
      perror("socket");
      exit(1);
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
}


char * generate(int length){
  char * list = malloc(length);
  char randomletter;
  for (size_t i = 0; i < length; i++) {
    randomletter = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[random () % 26];
    list[i] = randomletter;
  }
  return list;
}

void test(int octets){

  char * data = generate(octets);

  int ret = 0;

  char * buffer = malloc(octets*sizeof(char));
  char * empty = malloc(octets);
  int size = octets;

  printf("~~~~~~~~~~ TEST : Ecriture et Lecture de %d octets ~~~~~~~~~~~\n", octets);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  write(com[1], data, octets);
  read(com[0], buffer, octets);

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  free(buffer);
  buffer = malloc(octets*sizeof(char));

  printf("[pipe write/read] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  client_exchange(data, empty, octets);
  server_exchange(buffer, size);

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[socket send/recv] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  free(buffer);
  buffer = malloc(octets);

  if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror("clock");
      exit(1);
  }

  while(ret < octets){
    ret += conduct_write(conduit, data+ret, 100);
    conduct_read(conduit, buffer, 100);
  }

  if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror("clock");
      exit(1);
  }

  printf("[conduit write/read] Fait en %lf secondes\n", ( stop.tv_sec - start.tv_sec ) + (stop.tv_nsec - start.tv_nsec )/1.0e9);

  free(empty);
  free(buffer);
}

int main(int argc, char const *argv[]) {

  pipe(com);

  conduit = conduct_create(NULL, 100, 100000);

  start_server();
  start_client();

  test(100);
  test(200);
  test(300);
  test(400);
  test(500);
  test(1000);
  test(1100);
  test(1200);
  test(1300);
  test(1400);
  test(1500);
  test(10000);

  return 0;
}
