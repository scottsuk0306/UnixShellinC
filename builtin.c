#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

extern char **environ;

enum { FALSE, TRUE };


/*
  Builtin function implementations.
*/

int ish_setenv(int argc, char **args, char *filepath)
{
  if(argc == 1)
  {
    fprintf(stderr, "%s: setenv takes one or two parameters\n", filepath);
    return FALSE;
  }
  else if(argc >= 4)
  {
    fprintf(stderr, "%s: setenv takes one or two parameters\n", filepath);
    return FALSE;
  }
  char *name = args[1];
  char *value = "";
  if(argc == 3)
  {
    value = args[2];
  }
  setenv(name, value, 0);
  return TRUE;
}

int ish_unsetenv(int argc, char **args, char *filepath)
{
  // name is undeclared
  if(argc != 2)
  {
    fprintf(stderr, "%s: unsetenv takes one parameter\n", filepath);
    return FALSE;
  }
  char *name = args[1];
  unsetenv(name);
  return TRUE;
}


int ish_cd(int argc, char **args, char *filepath)
{
  if(argc == 1){
    chdir(getenv("HOME"));
  }
  else if(argc == 2){
    if(chdir(args[1]) == -1)
    {
      fprintf(stderr, "%s: No such file or directory\n", filepath);
      return FALSE; // Magic number. Has to be changed
    }
  }
  else{
    fprintf(stderr, "%s: cd takes one error\n", filepath);
    return FALSE;
  }
  return TRUE;
}

int ish_exit(void)
{
  exit(0);
}
