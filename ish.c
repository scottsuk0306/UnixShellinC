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

enum {MAX_LINE_SIZE = 1024};

enum {FALSE, TRUE};

char * filepath; // current file name (in this case, ./ish)

// pid_t wait(int *status);
// pid_t waitpid(pid_t pid, int *status, int options);


int builtin_Execute(int argc, const char* command, char **new_argv)
{
  if(!strcmp(command, "setenv"))
  {
    if(ish_setenv(argc, new_argv, filepath)) return EXIT_SUCCESS;
    return EXIT_FAILURE;
  }
  if(!strcmp(command, "unsetenv"))
  {
    if(ish_unsetenv(argc, new_argv, filepath)) return EXIT_SUCCESS;
    return EXIT_FAILURE;
  }
  if(!strcmp(command, "cd"))
  {
    if(ish_cd(argc, new_argv, filepath)) return EXIT_SUCCESS;
    return EXIT_FAILURE;
  }
  if(!strcmp(command, "exit"))
  {
    if(ish_exit()) return EXIT_SUCCESS;
    return EXIT_FAILURE;
  }

  return -1; // command is not a builtin command
}

int Execute(int argc, const char* command, char **new_argv)
{
  execvp(command, new_argv);
  // cat, ls, echo, and other commands
  // setenv unsetenv cd exit has to be realized
  fprintf(stderr, "%s: %s\n",command,strerror(errno));
  exit(EXIT_FAILURE);
}

/* static void tmHandler(int iSig)
{

}
*/

int Process(FILE *fp){
  FILE *fp2 = fp;
  char acLine[MAX_LINE_SIZE];
  DynArray_T oTokens;
  pid_t pid;
  int status;
  /*void (*pfRet)(int);
  pfRet = signal(SIGINT, myHandler);
  assert(pfRet != SIG_ERR);
 */
  while(TRUE)
  {
    if(fp2 == NULL)
    {
      fprintf(stdout,"%% ");
      if(fgets(acLine, MAX_LINE_SIZE, stdin) == NULL)  return FALSE;

    }
    else
    {
      fprintf(stdout,"%% ");
      if(fgets(acLine, MAX_LINE_SIZE, fp2) == NULL)  return FALSE;
      fprintf(stdout,"%s",acLine);
    }
    oTokens = DynArray_new(0);
    if (oTokens == NULL)
    {
      fprintf(stderr, "Cannot allocate memory\n"); // not sure about this
      exit(EXIT_FAILURE);
    }
    lexLine(acLine, oTokens);
    if(DynArray_getLength(oTokens) == 0) continue;
    synLine(oTokens);
    /* Parse command line
    Assign values to somepgm, someargv */

    /* If no tokens in the otokens, no need to execute */
    char **new_argv;
    int argc = DynArray_getLength(oTokens);
    new_argv = (char **)malloc(sizeof(char *)*(DynArray_getLength(oTokens)+1));
    struct Token ** array = (struct Token**)malloc(sizeof(struct Token*)*(DynArray_getLength(oTokens)));
    for(int i = 0; i < argc; i++)
    {
      array[i] = DynArray_get(oTokens,i);
      new_argv[i] = array[i] -> pcValue;
    }
    new_argv[argc] = NULL;
    const char * command = new_argv[0];
    fflush(NULL); // Your program should call fflush(NULL) before each call of fork to clear all I/O buffers.
    pid = fork();
    if (pid == 0)
    {
      // fprintf(stdout, "%s\n", new_argv);
      /* in child */
      if(!strcmp(command, "setenv")||!strcmp(command, "unsetenv")||!strcmp(command, "cd")||!strcmp(command, "exit"))
      {
        exit(EXIT_FAILURE);
      }
      Execute(argc, command, new_argv);
    }
    else if(pid < 0)
    {
      // exit the program
      perror(filepath);
      exit(EXIT_FAILURE);
    }
     /* in parent */
     /* in child, execvp automatically free the memory */
    //pfRet = signal(SIGINT, SIG_IGN);
    pid = wait(&status);
    builtin_Execute(argc, command, new_argv);
    DynArray_map(oTokens, freeToken, NULL);
    DynArray_free(oTokens);
    free(array);
    free(new_argv);
  }


}

int main(int argc, char **argv)
{
  filepath = argv[0];
  FILE * fp;
  fp = fopen(".ishrc","r");
  //fprintf(stdout,"%d",fp==NULL);
  if(fp)
  {
    //fprintf(stdout,"hello");
    Process(fp);
    fclose(fp);
  }
  /* If there is file named .ishrc */
  Process(NULL);
}
