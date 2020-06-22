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
  char *path = getenv("PWD");
  chdir(getenv("HOME"));
  fp = fopen(".ishrc","r");
  chdir(path);

  /* If there is file named .ishrc */
  if(fp)
  {
    Process_with_pipe(fp, filepath);
    fclose(fp);
  }
  Process_with_pipe(NULL, filepath);
}
