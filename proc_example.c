#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_PROC 3

int main(void) {
  int i;
  pid_t pid;

  for (i = 0; i < NUM_PROC; i++) {
    pid = fork();
    if(pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    }else if (pid == 0) {
      printf("Im the child, my pID is %jd, and my parent's is: %jd\n", (intmax_t)getpid(), (intmax_t)getppid());
      wait(NULL);
      exit(EXIT_SUCCESS);
    }else if (pid > 0) {
      printf("Im the parent, my pID is %jd\n", (intmax_t)getpid());
      wait(NULL);
    }
  }
  wait(NULL);
  exit(EXIT_SUCCESS);
}