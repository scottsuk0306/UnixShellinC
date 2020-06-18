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

/*--------------------------------------------------------------------*/

/* Same as setenv() in unistd.h. Print error if fails. */
int ish_setenv(int argc, char **args, char *filepath);

/*--------------------------------------------------------------------*/

/* Same as unsetenv() in unistd.h. Print error if fails. */
int ish_unsetenv(int argc, char **args, char *filepath);

/*--------------------------------------------------------------------*/

/* Same as cd() in unistd.h. Print error if fails. */
int ish_cd(int argc, char **args, char *filepath);

/*--------------------------------------------------------------------*/

/* Same as exit() in unistd.h. Print error if fails. */
int ish_exit(void);

/*--------------------------------------------------------------------*/
