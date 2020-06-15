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

enum TokenType {TOKEN_WORD, TOKEN_LINE};

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

static int Execute_with_pipe(int argc, char **argv, char *acLine, char *filepath)
{
  int status;
  int numPipes = 0;
  pid_t pid;

  for(int i = 0; i < argc; i++)
  {
    if(strcmp(argv[i], "|") == 0) numPipes++;
  }




  char *command_arr[numPipes+1];
  // first token for each cmd (line)
  // for example, if ls | grep .txt, then command[2] = ls, grep
  void (*pfRet) (int);
  pfRet = signal(SIGINT, SIG_IGN);
  pfRet = signal(SIGQUIT, quitHandler);
  pfRet = signal(SIGALRM, alarmHandler);
  // char *ptr = strtok(argv, "|");
  // int cmdIndex = 0;
  // while(ptr != NULL)
  // {
  //   assert(cmdIndex <= numPipes + 1);
  //   cmd_arr[cmdIndex] = ptr;
  //   ptr = strtok(NULL, "|");
  //   cmdIndex++;
  // }

  // ptr[0] = ["ls"]  ptr[1] = ["grep", "a.txt"]
  char **cmd_arr[numPipes+1]; //ptr[0] = new_argv
  // splice acLine
  // for example, if A | B | C, then cmd_arr[3] = {A, B, C}
  int cmdIndex = 0;
  int cmdNum = 0;
  for(int i = 0; i < argc; i++)
  {
    if(strcmp(argv[i],"|")!=0)
    {
      cmdNum++;
    }
    else
    {
      char *array[cmdNum+1];
      for(int j = 0; j < cmdNum; j++)
      {
        array[j] = argv[i-cmdNum+j];
      }
      array[cmdNum] = NULL;
      cmd_arr[cmdIndex++] = array;
      cmdNum = 0;
    }
  }

  // refresh cmdIndex
  cmdIndex = 0;

  for(int i = 0; i < argc; i++)
  {
    if(i == 0 || argv[i-1] == "|")
    {
      assert(cmdIndex <= numPipes + 1);
      command_arr[cmdIndex] = argv[i];
      cmdIndex++;
    }
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
          if(dup2(pipesfds[(cmdIndex-1)*2], 0) < 0)
          {
            fprintf(stderr, "%s: %s\n",filepath,strerror(errno));
            exit(EXIT_FAILURE);
          }
      }
      /* child outputs to next command, if it's not
          the last command */
      if(cmdIndex != numPipes)
      {
          if(dup2(pipesfds[cmdIndex*2+1], 1) < 0)
          {
            fprintf(stderr, "%s: %s\n",filepath,strerror(errno));
            exit(EXIT_FAILURE);
          }
      }

      for(int i = 0; i < 2*numPipes; i++)
      {
          close(pipesfds[i]);
      }

      fprintf(stdout, "%s", cmd_arr[cmdIndex]);

      if(!strcmp(command_arr[cmdIndex], "setenv")||!strcmp(command_arr[cmdIndex], "unsetenv")||!strcmp(command_arr[cmdIndex], "cd")||!strcmp(command_arr[cmdIndex], "exit"))
      {
        exit(EXIT_FAILURE); // to goto the parent process.
      }
      Execute(argc, cmd_arr[cmdIndex][0], cmd_arr[cmdIndex]); // A|B|C, A만 들어감
      //ls a.txt
      //["ls", "a.txt"]
      //ls | grep a.txt
      //["ls"]
      //["grep", "a.txt"]
      //["grep a.txt"]

    }
    else if(pid < 0)
    {
          fprintf(stderr, "%s: %s\n",filepath,strerror(errno));
          exit(EXIT_FAILURE);
    }

    wait(&status);
    builtin_Execute(argc, command_arr[cmdIndex], cmd_arr[cmdIndex], filepath);
    cmdIndex++;
  }

  /* parent closes all of its copies at the end */
  for(int i = 0; i < 2 * numPipes; i++ )
  {
      close(pipesfds[i]);
  }
  return 1;
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
    struct Token ** array = (struct Token**)malloc(sizeof(struct Token*)*(DynArray_getLength(oTokens)));
    for(int i = 0; i < argc; i++)
    {
      array[i] = DynArray_get(oTokens,i);
      new_argv[i] = array[i] -> pcValue;
    }
    new_argv[argc] = NULL;
    const char * command = new_argv[0];
    Execute_with_pipe(argc, new_argv, acLine, filepath);
    DynArray_map(oTokens, freeToken, NULL);
    DynArray_free(oTokens);
    free(array);
    free(new_argv);
  }
}
