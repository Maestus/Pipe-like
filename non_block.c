#include "conduct.h"
#include <time.h>
#include <unistd.h>

int discuss(int timer){
  pid_t pid;
  struct conduct *conduit =  conduct_create(NULL,50,100);
  for(int i=0;i<1;i++){
    pid = fork();
    if(pid<0)
      return -1;
    if(pid==0){
      sleep(i*1+1);
      break;
    }
  }
    printf("here1\n");
  if(pid !=0){
    time_t begin ;
    begin = time(NULL);
      char buf[5];
    while(1){
    if(try_conduct_read(conduit,buf,5)>0){
        printf("buffer fils %s",buf);
    }
        if(time(NULL)-begin> timer){
            printf("temps écoulé\n");
            return 0;
        }
    }
  }
  else{
      printf("la\n");
      if(conduct_write(conduit,"here\n",5) < 0){
          printf("error\n");
          return -1;
      }
      printf("sa sors\n");
    return 0;
  }
}

int main(){
  discuss(5);
}
