#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>



void freeToken(void *pvItem, void *pvExtra);

/* Free token pvItem.  pvExtra is unused. */

int lexLine(const char *pcLine, DynArray_T oTokens, char *filepath);

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return EXIT_SUCCESS if successful, or
   EXIT_FAILURE otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   pcLine. */

int synLine(const DynArray_T oTokens, char *filepath);
/* syntatically analyze the set of tokens. */

/* The '|' token should indicate that the immediate token
after the '|' is another command. If there's no following token after '|',
appropriate error message is printed.
There can be multiple pipe operators in a single command.
Return EXIT_SUCCESS if successful. Otherwise, return EXIT_FAILURE.
*/

int test(char *filepath);
/* test lexLine() and synLine() */
