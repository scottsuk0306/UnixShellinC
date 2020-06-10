#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "dynarray.h"

enum {MAX_LINE_SIZE = 1024};

enum {FALSE, TRUE};

// pid_t wait(int *status);
// pid_t waitpid(pid_t pid, int *status, int options);


int main(void)
{


  char acLine[MAX_LINE_SIZE];
  DynArray_T oTokens;
  pid_t pid;
  int status;


  while (TRUE)
  {
    printf("%% ");
    if(fgets(acLine, MAX_LINE_SIZE, stdin) == NULL)
    {
      return FALSE;
    }
    oTokens = DynArray_new(0);
    if (oTokens == NULL)
    {
      fprintf(stderr, "Cannot allocate memory\n");
      exit(EXIT_FAILURE);
    }
    lexLine(acLine, oTokens);
    synLine(oTokens);
    /* Parse command line
    Assign values to somepgm, someargv */
    char **new_argv;
    new_argv = (char **)malloc(sizeof(char *)*(DynArray_getLength(oTokens)+1));
    struct Token ** array = (struct Token**)malloc(sizeof(struct Token*)*(DynArray_getLength(oTokens)));
    for(int i = 0; i < DynArray_getLength(oTokens); i++)
    {
      array[i] = DynArray_get(oTokens,i);
      new_argv[i] = array[i] -> pcValue;
    }
    new_argv[DynArray_getLength(oTokens)] = NULL;

    const char * command = array[0] -> pcValue;
    fflush(NULL); // Your program should call fflush(NULL) before each call of fork to clear all I/O buffers.
    pid = fork();
    if (pid == 0)
    {
    /* in child */
      execvp(command, new_argv);
      // cat, ls, echo, and other commands
      // setenv unsetenv cd exit has to be realized
      fprintf(stderr, "%s: %s\n",command,strerror(errno));
      exit(EXIT_FAILURE);
    }
     /* in parent */
    pid = wait(&status);
    DynArray_map(oTokens, freeToken, NULL);
    DynArray_free(oTokens);
    free(array);
    free(new_argv);
  }
  /* Repeat the previous */
}
