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
#include "dynarray.h"

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

/*--------------------------------------------------------------------*/

/* If Ctrl-\ is typed within 5 seconds, terminate the process */
static void quitHandler2(int iSig)
{
   exit(EXIT_FAILURE);
}

/*--------------------------------------------------------------------*/

/* User type Ctrl-\ once */
static void quitHandler(int iSig)
{
  void (*pfRet) (int);
  pfRet = signal(SIGQUIT, quitHandler2);
  if(pfRet == SIG_ERR) exit(EXIT_FAILURE);
  alarm(5);
  fprintf(stdout,"\nType CTRl-\\ again within 5 seconds to exit.\n");
  fprintf(stderr, "%% ");
}

/*--------------------------------------------------------------------*/

/* User didn't type Ctrl-\ within 5 seconds */
static void alarmHandler(int iSig)
{
  void (*pfRet) (int);
  pfRet = signal(SIGQUIT, quitHandler);
  if(pfRet == SIG_ERR) exit(EXIT_FAILURE);
}

/*--------------------------------------------------------------------*/

/* Implement builtin execution */
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

/*--------------------------------------------------------------------*/

/* Implement execution */
static int Execute(int argc, const char* command, char **new_argv)
{
  execvp(command, new_argv);
  // cat, ls, echo, and other commands
  // setenv unsetenv cd exit has to be realized
  fprintf(stderr, "%s: %s\n",command,strerror(errno));
  exit(EXIT_FAILURE);
}

/*--------------------------------------------------------------------*/

/* If '<' token exists, redirect standard input */
static void InputRD(const char *fd)
{
  int iFd;
  int iRet;

  iFd = open(fd, O_RDWR );
  if(iFd == -1)
  {
    fprintf(stderr, "No such file or directory\n");
    exit(EXIT_FAILURE);
  }
  iRet = dup2(iFd, STDIN_FILENO);
  if (iRet == -1) exit(EXIT_FAILURE);
  iRet = close(iFd);
}

/*--------------------------------------------------------------------*/

/* If '>' token exists, redirect standard output */
static void OutputRD(const char *fd)
{
  int iFd;
  int iRet;

  iFd = creat(fd, 0600);
  iRet = dup2(iFd, STDOUT_FILENO);
  if (iRet == -1) exit(EXIT_FAILURE);
  iRet = close(iFd);
}

/*--------------------------------------------------------------------*/

/* If '|' token exists, implement execution with this function */
static int Execute_with_pipe(int argc, char **argv, char *acLine, char *filepath, DynArray_T oTokens) // builtin_Execute + Execute + pipe
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
  if(pfRet == SIG_ERR) exit(EXIT_FAILURE);


  struct Token ** pipe_count = (struct Token**)malloc(sizeof(struct Token*)*(DynArray_getLength(oTokens)));
  if(pipe_count == NULL)
  {
    fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
    exit(EXIT_FAILURE);
  }
  int pExits[argc+1];
  int spExits[argc+1];
  for(int i = 0; i < argc; i++)
  {
    pipe_count[i] = DynArray_get(oTokens, i);
    if(pipe_count[i] -> eType == TOKEN_LINE)
    {
      numPipes++;
      pExits[i] = 1;
    }
    else pExits[i] = 0;
  }
  for(int i = 0; i < argc; i++)
  {
    if(pipe_count[i] -> eType == TOKEN_SPECIAL)
    {
      spExits[i] = 1;
    }
    else spExits[i] = 0;
  }
  free(pipe_count);
  char line_arr[numPipes+1][MAX_LINE_SIZE];
  int lineIndex = 0;
  strcpy(line_arr[lineIndex],"\0");
  if(strstr(argv[0], " ")||strstr(argv[0], "\t"))
  {
    strcat(line_arr[lineIndex],"\"");
    strcat(line_arr[lineIndex], argv[0]);
    strcat(line_arr[lineIndex],"\"");

  }
  else
  {
    strcpy(line_arr[lineIndex], argv[0]);
  }
  for(int i = 1; i < argc; i++)
  {
    if(!strcmp(argv[i], "|") && pExits[i] == 1)
    {
      lineIndex++;
      strcpy(line_arr[lineIndex], "\0");
    }
    else
    {
      strcat(line_arr[lineIndex], " ");
      if((!strcmp(argv[i], "|"))||(!strcmp(argv[i], ">") && spExits[i]!=1)||(!strcmp(argv[i], "<") && spExits[i]!=1)||strstr(argv[i], " ")||strstr(argv[i], "\t"))
      {
        strcat(line_arr[lineIndex],"\"");
        strcat(line_arr[lineIndex], argv[i]);
        strcat(line_arr[lineIndex],"\"");
      }
      else
      {
        strcat(line_arr[lineIndex], argv[i]);
      }
    }
  }
  int cmdIndex = 0;

  // while(ptr != NULL)
  // {
  //   assert(cmdIndex <= numPipes + 1);
  //   line_arr[cmdIndex] = ptr;
  //   ptr = strtok(NULL, "|");
  //   cmdIndex++;
  // }
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
    if(new_argv[cmdIndex] == NULL)
    {
      fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
    // +1 because new_argv is a string. It has to end with NULL.
    struct Token ** array = (struct Token**)malloc(sizeof(struct Token*)*(DynArray_getLength(oTokens)));
    if(array == NULL)
    {
      fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
    input_RD[cmdIndex] = 0;
    output_RD[cmdIndex] = 0;
    int new_argc = 0;

    for(int i = 0; i < argc; i++)
    {
      array[i] = DynArray_get(oTokens,i);
    }
    for(int i = 0; i < argc; i++)
    {
      if(array[i]->eType == TOKEN_SPECIAL && !strcmp(array[i] -> pcValue, "<"))
      {
        input_RD[cmdIndex] = 1;
        filename_in = strdup(array[i+1] -> pcValue);
        if(filename_in == NULL)
        {
          fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
          exit(EXIT_FAILURE);
        }
      }
      else if(array[i]->eType == TOKEN_SPECIAL && !strcmp(array[i] -> pcValue, ">"))
      {
        output_RD[cmdIndex] = 1;
        filename_out = strdup(array[i+1] -> pcValue);
        if(filename_out == NULL)
        {
          fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
          exit(EXIT_FAILURE);
        }
      }
      else if(i > 1 && array[i-1] -> eType == TOKEN_SPECIAL)
      {
        continue;
      }
      else
      {
        new_argv[cmdIndex][new_argc++] = strdup(array[i] -> pcValue);
        // if(new_argv[cmdIndex][new_argc] == NULL)
        // {
        //   fprintf(stderr,"%s: 7 Cannot allocate memory\n", filepath);
        //   exit(EXIT_FAILURE);
        // }

      }
    }
    new_argv[cmdIndex][new_argc] = NULL;

    cmd_arr[cmdIndex] = new_argv[cmdIndex];
    command_arr[cmdIndex] = strdup(new_argv[cmdIndex][0]);
    if(command_arr[cmdIndex] == NULL)
    {
      fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
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
      if(pfRet == SIG_ERR) exit(EXIT_FAILURE);
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

  for(int i = 0; i < numPipes + 1; i++)
  {
    for(int j = 0; new_argv[i][j]!=NULL; j++)
    {
      free(new_argv[i][j]);
    }
    free(new_argv[i]);
  }
  for(int i = 0; i < numPipes + 1; i++)
  {
    free(command_arr[i]);
  }
  //assert(0);
  /* parent closes all of its copies at the end */
  return EXIT_SUCCESS;
}

/*--------------------------------------------------------------------*/

/* Get input from stdin or file and implement Unix processes */
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
  if(pfRet == SIG_ERR) exit(EXIT_FAILURE);
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
      if(pfRet == SIG_ERR) exit(EXIT_FAILURE);
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

/*--------------------------------------------------------------------*/

int Process_with_pipe(FILE *fp, char *filepath)
{
  char acLine[MAX_LINE_SIZE];
  DynArray_T oTokens;
  void (*pfRet) (int);
  pfRet = signal(SIGINT, SIG_IGN);
  pfRet = signal(SIGQUIT, quitHandler);
  pfRet = signal(SIGALRM, alarmHandler);
  if(pfRet == SIG_ERR) exit(EXIT_FAILURE);
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
    if(new_argv == NULL)
    {
      fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
    struct Token ** array = (struct Token**)malloc(sizeof(struct Token*)*(DynArray_getLength(oTokens)));
    if(array == NULL)
    {
      fprintf(stderr,"%s: Cannot allocate memory\n", filepath);
      exit(EXIT_FAILURE);
    }
    for(int i = 0; i < argc; i++)
    {
      array[i] = DynArray_get(oTokens,i);
      new_argv[i] = array[i] -> pcValue;
    }
    new_argv[argc] = NULL;
    Execute_with_pipe(argc, new_argv, acLine, filepath, oTokens);
    free(array);
    free(new_argv);
  }
}

/*--------------------------------------------------------------------*/
