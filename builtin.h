#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

extern char **environ;

/*
  List of builtin commands, followed by their corresponding functions.
*/
char *builtin_str[] = {
  "setenv",
  "unsetenv",
  "cd",
  "exit"
};

int ish_setenv(int argc, char **args, char *filepath);

int ish_unsetenv(int argc, char **args, char *filepath);

int ish_cd(int argc, char **args, char *filepath);

int ish_exit(void);
