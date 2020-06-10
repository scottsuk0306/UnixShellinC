/*--------------------------------------------------------------------*/
/* dfa.c                                                              */
/* Original Author: Bob Dondero                                       */
/* Illustrate lexical analysis using a deterministic finite state     */
/* automaton (DFA)                                                    */
/*--------------------------------------------------------------------*/

#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------------------*/

enum {MAX_LINE_SIZE = 1024};

enum {FALSE, TRUE};

enum TokenType {TOKEN_WORD, TOKEN_LINE};

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

void freeToken(void *pvItem, void *pvExtra) // static is deleted

/* Free token pvItem.  pvExtra is unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   free(psToken->pcValue);
   free(psToken);
}

/*--------------------------------------------------------------------*/

static void printWordToken(void *pvItem, void *pvExtra)

/* Print token pvItem to stdout iff it is a word.  pvExtra is
   unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   if (psToken->eType == TOKEN_WORD)
      printf("%s ", psToken->pcValue);
}

/*--------------------------------------------------------------------*/

static void printLineToken(void *pvItem, void *pvExtra)

/* Print token pvItem to stdout iff it is a Line.  pvExtra is
   unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   if (psToken->eType == TOKEN_LINE)
      printf("%s ", psToken->pcValue);
}

/*--------------------------------------------------------------------*/

static void printAllToken(void *pvItem, void *pvExtra)

/* Print token pvItem to stdout.  pvExtra is
   unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   printf("%s\n", psToken->pcValue);
}

/*--------------------------------------------------------------------*/

static struct Token *makeToken(enum TokenType eTokenType,
   char *pcValue)

/* Create and return a Token whose type is eTokenType and whose
   value consists of string pcValue.  Return NULL if insufficient
   memory is available.  The caller owns the Token. */

{
   struct Token *psToken;

   psToken = (struct Token*)malloc(sizeof(struct Token));
   if (psToken == NULL)
      return NULL;

   psToken->eType = eTokenType;

   psToken->pcValue = (char*)malloc(strlen(pcValue) + 1);
   if (psToken->pcValue == NULL)
   {
      free(psToken);
      return NULL;
   }

   strcpy(psToken->pcValue, pcValue);

   return psToken;
}

/*--------------------------------------------------------------------*/

int lexLine(const char *pcLine, DynArray_T oTokens)

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   pcLine. */

{
   enum LexState {STATE_START, STATE_IN_WORD, STATE_IN_COMM, STATE_IN_LINE};

   enum LexState eState = STATE_START;

   int iLineIndex = 0;
   int iValueIndex = 0;
   char c;
   char acValue[MAX_LINE_SIZE];
   struct Token *psToken;

   assert(pcLine != NULL);
   assert(oTokens != NULL);

   for (;;)
   {
      /* "Read" the next character from pcLine. */
      c = pcLine[iLineIndex++];

      switch (eState)
      {
         case STATE_START:
            if ((c == '\n') || (c == '\0'))
               return TRUE;

            else if (c == '"')
            {
              eState = STATE_IN_COMM;
            }

            else if (c == '|')
            {
              acValue[iValueIndex++] = c;
              eState = STATE_IN_LINE;
            }

            else if ( isspace(c) || (c == '\t'))
               eState = STATE_START;

            else if (isalpha(c) || (c > 32 && c < 127)) // || isdigit(c)
            {
              acValue[iValueIndex++] = c;
              eState = STATE_IN_WORD;
            }
            else
            {
               fprintf(stderr, "Invalid line\n");
               return FALSE;
            }

            break;

         case STATE_IN_WORD:
            if ((c == '\n') || (c == '\0'))
            {
               /* Create a WORD token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               iValueIndex = 0;

               return TRUE;
            }
            else if (c == '"')
            {
               eState = STATE_IN_COMM;
            }

            else if (c == '|')
            {
               /* Create a Word token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_LINE;
            }

            else if ( isspace(c) || (c == '\t'))
            {
               /* Create a WORD token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               iValueIndex = 0;

               eState = STATE_START;
            }

            else if (isalpha(c) || (c > 32 && c < 127))
            {
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }

            else
            {
               fprintf(stderr, "Invalid line\n");
               return FALSE;
            }

            break;

         case STATE_IN_COMM:
            if (c == '"')
            {
               eState = STATE_IN_WORD;
            }

            else if (c == '\0')
            {
               fprintf(stderr, "ERROR - unmatched quote\n");
               return FALSE;
            }

            else
            {
               acValue[iValueIndex++] = c;
               eState = STATE_IN_COMM;
            }

            break;

         case STATE_IN_LINE:
            if ((c == '\n') || (c == '\0'))
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }
               iValueIndex = 0;

               return TRUE;
            }

            else if (c == '"')
            {
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               iValueIndex = 0;

               eState = STATE_IN_COMM;
            }

            else if (c == '|')
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);
               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_LINE;
            }

            else if ( isspace(c) || (c == '\t'))
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               iValueIndex = 0;

               eState = STATE_START;
            }

            else if (isalpha(c) || (c > 32 && c < 127))
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "Cannot allocate memory\n");
                  return FALSE;
               }
               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }

            else
            {
               fprintf(stderr, "Invalid line\n");
               return FALSE;
            }

            break;

         default:
            assert(FALSE);
      }
   }
}

/*--------------------------------------------------------------------*/
int synLine(DynArray_T oTokens)
/* syntatically analyze the set of tokens. */

/* The '|' token should indicate that the immediate token
after the '|' is another command. If there's no following token after '|',
appropriate error message is printed.
There can be multiple pipe operators in a single command.
Return 1 if successful. Otherwise, return 0.
 */

{
  assert(oTokens != NULL);

  struct Token ** array = (struct Token**)malloc(DynArray_getLength(oTokens)*sizeof(struct Token*));
  for(int i = 0; i < DynArray_getLength(oTokens); i++)
  {
    array[i] = DynArray_get(oTokens,i);
  }
  for(int i = 0; i < DynArray_getLength(oTokens); i++)
  {
    if(array[i] -> eType == TOKEN_LINE)
    {
      if(i == 0) // first is |
      {
        fprintf(stderr, "Missing command name");
        free(array);
        return FALSE;
      }
      if(i == DynArray_getLength(oTokens)-1) // last is |
      {
        fprintf(stderr, "Missing command name");
        free(array);
        return FALSE;
      }
      else if(array[i+1] -> eType == TOKEN_LINE) // command should follow after |, no ||
      {
        fprintf(stderr, "Pipe or redirection destination not specified");
        free(array);
        return FALSE;
      }
    }
  }
  free(array);
  return TRUE;

}


/*--------------------------------------------------------------------*/

int test(void)

/* Function that test lexical and syntatic analysis.
  Read a line from stdin, and write to stdout each number and word
   that it contains.  Repeat until EOF.  Return 0 iff successful. */

{
   char acLine[MAX_LINE_SIZE];
   DynArray_T oTokens;
   int iSuccessful;

   printf("------------------------------------\n");
   while (fgets(acLine, MAX_LINE_SIZE, stdin) != NULL)
   {
      oTokens = DynArray_new(0);
      if (oTokens == NULL)
      {
         fprintf(stderr, "Cannot allocate memory\n");
         exit(EXIT_FAILURE);
      }

      iSuccessful = lexLine(acLine, oTokens);
      if (iSuccessful)
      {
          printf("Words:  ");
          DynArray_map(oTokens, printWordToken, NULL);
          printf("\n");

          printf("Line: ");
          DynArray_map(oTokens, printLineToken, NULL);
          printf("\n");

          printf("Result: \n");
          DynArray_map(oTokens, printAllToken, NULL);
          printf("\n");

      }
      printf("------------------------------------\n");

      synLine(oTokens);
      printf("\n");

      printf("------------------------------------\n");

      DynArray_map(oTokens, freeToken, NULL);
      DynArray_free(oTokens);
   }

   return 0;
}
