#include "conduct.h"
#include <time.h>
#include <unistd.h>

int discuss(int timer){
  pid_t pid;
  struct conduct *conduit =  conduct_create(NULL,100,50);
  for(int i=0;i<10;i++){
    pid = fork();
    if(pid<0)
      return -1;
    if(pid==0){
      sleep(i*10);
      break;
    }
  }
    printf("here1\n");
  if(pid !=0){
    time_t begin ;
    begin = time(NULL);
      char buf[5];
      printf("dans le père\n");
    while(1){
        printf("avant la lecture\n");
    if(try_conduct_read(conduit,buf,5)>=0){
        printf("buf %s",buf);
    }
        printf("après\n");
      if(time(NULL)-begin> timer)
        return 0;
    }
  }
  else{
    if(conduct_write(conduit,"here\n",5) < 0)
      return -1;
    return 0;
  }
}

int main(){
  discuss(100);
}
