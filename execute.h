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
#include "dynarray.h"
#include "builtin.h"
#include "signal.h"

/*--------------------------------------------------------------------*/

/* ish loop without special characters like "|", "<", and ">" */
int Process(FILE *fp, char *filepath);

/*--------------------------------------------------------------------*/

/* ish loop with special characters like "|", "<", and ">" */
int Process_with_pipe(FILE *fp, char *filepath);

/*--------------------------------------------------------------------*/
