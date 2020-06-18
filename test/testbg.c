#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <wait.h>
int main()
{
  chdir("../");
  chdir("/usr/bin");
  char *ls_ag[] = {"ls","-al", NULL};
  char *grep_ag[] = {"grep", "fwe", NULL};
  int fd[2];
  pipe(fd);
  if(fork() == 0)
    {
      close(fd[0]);
      dup2(fd[1],1);
      execvp(*ls_ag,ls_ag);
    }
  else{
    close(fd[1]);
    wait(NULL);
    if(fork() == 0)
      {
   close(fd[1]);
   dup2(fd[0],0);
   execvp(*grep_ag, grep_ag);
      }
    else
      {
   close(fd[0]);
   wait(NULL);
      }
  }
    return 0;
}
