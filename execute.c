#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "builtin.h"
#include "execute.h"

enum TokenType {TOKEN_WORD, TOKEN_LINE, TOKEN_SPECIAL};

enum {MAX_LINE_SIZE = 1024};

enum {FALSE, TRUE};

/*--------------------------------------------------------------------*/

/* A Token is either a number or a word, expressed as a string. */

struct Token
{
   enum TokenType eType;
   /* The type of the token. */

   char *pcValue;
   /* The string which is the token's value. */
};

static void quitHandler2(int iSig)
{
   exit(EXIT_FAILURE);
}

static void quitHandler(int iSig)
{
  void (*pfRet) (int);
  pfRet = signal(SIGQUIT, quitHandler2);
  alarm(5);
  fprintf(stdout,"\nType CTRl-\\ again within 5 seconds to exit.\n");
  fprintf(stderr, "%% ");
}

static void alarmHandler(int iSig)
{
  void (*pfRet) (int);
  pfRet = signal(SIGQUIT, quitHandler);
}


static int builtin_Execute(int argc, const char* command, char **new_argv, char *filepath)
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

static int Execute(int argc, const char* command, char **new_argv)
{
  execvp(command, new_argv);
  // cat, ls, echo, and other commands
  // setenv unsetenv cd exit has to be realized
  fprintf(stderr, "%s: %s\n",command,strerror(errno));
  exit(EXIT_FAILURE);
}

// A < B | C > D

static void InputRD(const char *fd)
{
  int iFd;
  int iRet;

  iFd = open(fd, O_RDWR, S_IRWXU);
  if(iFd == -1)
  {
    fprintf(stderr, "No such file or directory\n");
    exit(EXIT_FAILURE);
  }
  iRet = dup2(iFd, STDIN_FILENO);
  iRet = close(iFd);
}

static void OutputRD(const char *fd)
{
  // assert(0);
  int iFd;
  int iRet;

  iFd = creat(fd, S_IRWXU);
  iRet = dup2(iFd, STDOUT_FILENO);
  iRet = close(iFd);
}

static int Execute_with_pipe(int argc, char **argv, char *acLine, char *filepath) // builtin_Execute + Execute + pipe
{
  int status;
  int numPipes = 0;
  pid_t pid;


  // first token for each cmd (line)
  // for example, if ls | grep .txt, then command[2] = ls, grep
  void (*pfRet) (int);
  pfRet = signal(SIGINT, SIG_IGN);
  pfRet = signal(SIGQUIT, quitHandler);
  pfRet = signal(SIGALRM, alarmHandler);

  for(int i = 0; i < argc; i++)
  {
    if(strcmp(argv[i], "|") == 0) numPipes++;
  }

  char *line_arr[numPipes+2];
  char *ptr = strtok(acLine, "|");
  int cmdIndex = 0;

  while(ptr != NULL)
  {
    assert(cmdIndex <= numPipes + 1);
    line_arr[cmdIndex] = ptr;
    ptr = strtok(NULL, "|");
    cmdIndex++;
  }
  int input_RD[numPipes + 1];
  int output_RD[numPipes + 1];

  cmdIndex = 0;

  char **cmd_arr[numPipes+1];
  char *command_arr[numPipes+1];
  char *filename_in;
  char *filename_out;

  char **new_argv[numPipes+1];
  for(int i = 0; i < numPipes + 1; i++)
  {
    DynArray_T oTokens = DynArray_new(0);
    if (oTokens == NULL)
    {
      fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
    if(lexLine(line_arr[i], oTokens, filepath) == EXIT_FAILURE) continue;
    if(DynArray_getLength(oTokens) == 0) continue;
    if(synLine(oTokens, filepath) == EXIT_FAILURE) continue;
    /* Parse command line
    Assign values to somepgm, someargv */

    /* If no tokens in the otokens, no need to execute */

    int argc = DynArray_getLength(oTokens);
    new_argv[cmdIndex] = (char **)malloc(sizeof(char *)*(DynArray_getLength(oTokens)+1));
    // +1 because new_argv is a string. It has to end with NULL.
    struct Token ** array = (struct Token**)malloc(sizeof(struct Token*)*(DynArray_getLength(oTokens)));

    input_RD[cmdIndex] = 0;
    output_RD[cmdIndex] = 0;
    int new_argc = 0;

    for(int i = 0; i < argc; i++)
    {
      array[i] = DynArray_get(oTokens,i);
    }
    for(int i = 0; i < argc; i++)
    {
      if(!strcmp(array[i] -> pcValue, "<"))
      {
        input_RD[cmdIndex] = 1;
        filename_in = strdup(array[i+1] -> pcValue);
      }
      else if(!strcmp(array[i] -> pcValue, ">"))
      {
        output_RD[cmdIndex] = 1;
        filename_out = strdup(array[i+1] -> pcValue);
      }
      else if(i > 1 && array[i-1] -> eType == TOKEN_SPECIAL)
      {
        continue;
      }
      else
      {
        new_argv[cmdIndex][new_argc++] = strdup(array[i] -> pcValue);
      }
      //fprintf(stderr, "new_argv %d : %s\n", i, new_argv[i]);
    }
    new_argv[cmdIndex][new_argc] = NULL;
    // char * command = new_argv[0];

    cmd_arr[cmdIndex] = new_argv[cmdIndex];
    command_arr[cmdIndex] = strdup(new_argv[cmdIndex][0]);
    //fprintf(stderr, "cmd_arr %d : %s\n", cmdIndex, cmd_arr[cmdIndex][0]);
    // assert(0);
    cmdIndex++;
    DynArray_map(oTokens, freeToken, NULL);
    DynArray_free(oTokens);
    free(array);
  }

  if(!strcmp(command_arr[0], "setenv")||!strcmp(command_arr[0], "unsetenv")||!strcmp(command_arr[0], "cd")||!strcmp(command_arr[0], "exit"))
  {
    builtin_Execute(argc, command_arr[0], argv, filepath);
    return EXIT_SUCCESS;
  }

  // refresh cmdIndex
  cmdIndex = 0;
  // make multiple pipes for each "|"
  int pipesfds[2*numPipes];

  /* parent creates all needed pipes at the start */
  for(int i = 0; i < numPipes; i++)
  {
      if(pipe(pipesfds + i*2) < 0)
      {
          fprintf(stderr, "%s: %s\n",filepath,strerror(errno));
          exit(EXIT_FAILURE);
      }
  }

  while(cmdIndex < numPipes + 1)
  {
    fflush(NULL); // Your program should call fflush(NULL) before each call of fork to clear all I/O buffers.
    pid = fork();
    if(pid == 0)
    {

      pfRet = signal(SIGINT, SIG_DFL);
      pfRet = signal(SIGQUIT, SIG_DFL);

      /* child gets input from the previous command,
          if it's not the first command */
      if(cmdIndex != 0)
      {
          if(dup2(pipesfds[(cmdIndex-1)*2], STDIN_FILENO) < 0)
          {
            fprintf(stderr, "%s: %s\n",filepath,strerror(errno));
            exit(EXIT_FAILURE);
          }
      }
      /* child outputs to next command, if it's not
          the last command */
      if(cmdIndex != numPipes)
      {
          if(dup2(pipesfds[cmdIndex*2+1], STDOUT_FILENO) < 0)
          {
            fprintf(stderr, "%s: %s\n",filepath,strerror(errno));
            exit(EXIT_FAILURE);
          }
      }

      for(int i = 0; i < 2*numPipes; i++)
      {
          close(pipesfds[i]);
      }

      // cd does not use pipes
      if (input_RD[cmdIndex]) //input redirection
      {
        InputRD(filename_in);
      }
      if(output_RD[cmdIndex])
      {
        OutputRD(filename_out);
      }
      Execute(argc, command_arr[cmdIndex], cmd_arr[cmdIndex]); // A|B|C, A만 들어감
    }
    else if(pid < 0)
    {
          fprintf(stderr, "%s: %s\n",filepath,strerror(errno));
          exit(EXIT_FAILURE);
    }

    cmdIndex++;
  }
  for(int i = 0; i < 2 * numPipes; i++ )
  {
      close(pipesfds[i]);
  }
  for(int i = 0; i < numPipes + 1; i++)
  {
    wait(&status);
  }

  // fprintf(stderr, "finished\n");
  /* parent closes all of its copies at the end */

  return EXIT_SUCCESS;
}

int Process(FILE *fp, char *filepath)
{
  char acLine[MAX_LINE_SIZE];
  DynArray_T oTokens;
  pid_t pid;
  int status;
  void (*pfRet) (int);
  pfRet = signal(SIGINT, SIG_IGN);
  pfRet = signal(SIGQUIT, quitHandler);
  pfRet = signal(SIGALRM, alarmHandler);
  while(TRUE)
  {
    if(fp == NULL)
    {
      fprintf(stdout,"%% ");
      if(fgets(acLine, MAX_LINE_SIZE, stdin) == NULL)  return FALSE;

    }
    else
    {
      fprintf(stdout,"%% ");
      if(fgets(acLine, MAX_LINE_SIZE, fp) == NULL)  return FALSE;
      fprintf(stdout,"%s",acLine);
    }
    oTokens = DynArray_new(0);
    if (oTokens == NULL)
    {
      fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
    if(lexLine(acLine, oTokens, filepath) == EXIT_FAILURE) continue;
    if(DynArray_getLength(oTokens) == 0) continue;
    if(synLine(oTokens, filepath) == EXIT_FAILURE) continue;
    /* Parse command line
    Assign values to somepgm, someargv */

    /* If no tokens in the otokens, no need to execute */
    char **new_argv;
    int argc = DynArray_getLength(oTokens);
    new_argv = (char **)malloc(sizeof(char *)*(DynArray_getLength(oTokens)+1));
    // +1 because new_argv is a string. It has to end with NULL.
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
      /* in child */
      pfRet = signal(SIGINT, SIG_DFL);
      pfRet = signal(SIGQUIT, SIG_DFL);
      if(!strcmp(command, "setenv")||!strcmp(command, "unsetenv")||!strcmp(command, "cd")||!strcmp(command, "exit"))
      {
        exit(EXIT_FAILURE); // to goto the parent process.
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
    builtin_Execute(argc, command, new_argv, filepath);
    DynArray_map(oTokens, freeToken, NULL);
    DynArray_free(oTokens);
    free(array);
    free(new_argv);
  }
}

int Process_with_pipe(FILE *fp, char *filepath)
{
  char acLine[MAX_LINE_SIZE];
  DynArray_T oTokens;
  void (*pfRet) (int);
  pfRet = signal(SIGINT, SIG_IGN);
  pfRet = signal(SIGQUIT, quitHandler);
  pfRet = signal(SIGALRM, alarmHandler);
  while(TRUE)
  {
    if(fp == NULL)
    {
      fprintf(stdout,"%% ");
      if(fgets(acLine, MAX_LINE_SIZE, stdin) == NULL)  return FALSE;

    }
    else
    {
      fprintf(stdout,"%% ");
      if(fgets(acLine, MAX_LINE_SIZE, fp) == NULL)  return FALSE;
      fprintf(stdout,"%s",acLine);
    }
    oTokens = DynArray_new(0);
    if (oTokens == NULL)
    {
      fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
    if(lexLine(acLine, oTokens, filepath) == EXIT_FAILURE) continue;
    if(DynArray_getLength(oTokens) == 0) continue;
    if(synLine(oTokens, filepath) == EXIT_FAILURE) continue;
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
    Execute_with_pipe(argc, new_argv, acLine, filepath);
    DynArray_map(oTokens, freeToken, NULL);
    DynArray_free(oTokens);
    free(array);
    free(new_argv);
  }
}
