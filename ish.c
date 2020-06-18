#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "dynarray.h"
#include "builtin.h"
#include <signal.h>
#include "execute.h"

enum {MAX_LINE_SIZE = 1024};


char * filepath; // current file name (in this case, ./ish)

// pid_t wait(int *status);
// pid_t waitpid(pid_t pid, int *status, int options);

int main(int argc, char **argv)
{
  sigset_t sSet;
  sigemptyset(&sSet);
  sigaddset(&sSet, SIGINT);
  sigaddset(&sSet, SIGQUIT);
  sigaddset(&sSet, SIGALRM);
  sigprocmask(SIG_UNBLOCK, &sSet, NULL);
  filepath = argv[0];
  FILE * fp;
  fp = fopen(".ishrc","r");
  //fprintf(stdout,"%d",fp==NULL);
  if(fp)
  {
    //fprintf(stdout,"hello");
    Process_with_pipe(fp, filepath);
    fclose(fp);
  }
  /* If there is file named .ishrc */
  Process_with_pipe(NULL, filepath);
}
