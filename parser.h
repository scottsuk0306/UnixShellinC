#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

enum TokenType {TOKEN_WORD, TOKEN_LINE};

struct Token
{
   enum TokenType eType;
   /* The type of the token. */

   char *pcValue;
   /* The string which is the token's value. */
};

void freeToken(void *pvItem, void *pvExtra);

/* Free token pvItem.  pvExtra is unused. */

int lexLine(const char *pcLine, DynArray_T oTokens);

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   pcLine. */

int synLine(DynArray_T oTokens);
/* syntatically analyze the set of tokens. */

/* The '|' token should indicate that the immediate token
after the '|' is another command. If there's no following token after '|',
appropriate error message is printed.
There can be multiple pipe operators in a single command.
Return 1 if successful. Otherwise, return 0.
*/
