#include "../conduct.h"
#include <time.h>
#include <unistd.h>

int discuss(int timer){
  pid_t pid;
  struct conduct *conduit =  conduct_create(NULL,50,100);
  for(int i=0;i<8;i++){
    pid = fork();
    if(pid<0)
      return -1;
    if(pid==0){
      sleep(i*1+1);
      break;
    }
  }
  if(pid !=0){
    time_t begin ;
    begin = time(NULL);
      char buf[5];
    while(1){
    if(try_conduct_read(conduit,buf,5)>0){
        printf("buffer fils %s\n",buf);
    }
        if(time(NULL)-begin> timer){
            printf("temps écoulé\n");
            return 0;
        }
    }
  }
  else{
    char * num = malloc(5);
    sprintf(num,"%u\n", getpid());
    if(conduct_write(conduit, num, 5) < 0){
          printf("error\n");
          return -1;
      }
    return 0;
  }
}

int main(){
  discuss(5);
}
