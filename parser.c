#include "dynarray.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*--------------------------------------------------------------------*/

enum {MAX_LINE_SIZE = 1024};

enum TokenType {TOKEN_WORD, TOKEN_LINE, TOKEN_SPECIAL}; // TOKEN_SPECIAL

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

static void printSpecialToken(void *pvItem, void *pvExtra)

/* Print token pvItem to stdout iff it is a Line.  pvExtra is
   unused. */

{
   struct Token *psToken = (struct Token*)pvItem;
   if (psToken->eType == TOKEN_SPECIAL)
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

int lexLine(const char *pcLine, DynArray_T oTokens, char *filepath)

/* Lexically analyze string pcLine.  Populate oTokens with the
   tokens that pcLine contains.  Return 1 (TRUE) if successful, or
   0 (FALSE) otherwise.  In the latter case, oTokens may contain
   tokens that were discovered before the error. The caller owns the
   tokens placed in oTokens. */

/* lexLine() uses a DFA approach.  It "reads" its characters from
   pcLine. */

{
   enum LexState {STATE_START, STATE_IN_WORD, STATE_IN_COMM, STATE_IN_LINE, STATE_IN_SPECIAL};

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
               return EXIT_SUCCESS;

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

            //else if (isalpha(c) || (c > 32 && c < 127)) // || isdigit(c)
            else if ( c == '<' || c == '>')
            {
              acValue[iValueIndex++] = c;
              eState = STATE_IN_SPECIAL;
            }

            else
            {
              acValue[iValueIndex++] = c;
              eState = STATE_IN_WORD;
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
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;

               return EXIT_SUCCESS;
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
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_LINE;
            }

            else if (isspace(c) || (c == '\t'))
            {
               /* Create a WORD token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_WORD, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;

               eState = STATE_START;
            }
            else if ( c == '<' || c == '>')
            {

              acValue[iValueIndex] = '\0';
              psToken = makeToken(TOKEN_WORD, acValue);

              if (psToken == NULL)
              {
                 fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                 return EXIT_FAILURE;
              }

              if (! DynArray_add(oTokens, psToken))
              {
                 fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                 return EXIT_FAILURE;
              }

              iValueIndex = 0;
              acValue[iValueIndex++] = c;
              eState = STATE_IN_SPECIAL;
            }
            // else if (isalpha(c) || (c > 32 && c < 127))
            else
            {
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }

            break;

         case STATE_IN_COMM:
            if (c == '"')
            {
               eState = STATE_IN_WORD;
            }

            else if (c == '\0')
            {
               fprintf(stderr, "%s: Could not find quote pair\n",filepath);
               return EXIT_FAILURE;
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
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }
               iValueIndex = 0;

               return EXIT_SUCCESS;
            }

            else if (c == '"')
            {
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
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
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_LINE;
            }

            else if ( c == '<' || c == '>')
            {
              acValue[iValueIndex] = '\0';
              psToken = makeToken(TOKEN_LINE, acValue);

              if (psToken == NULL)
              {
                 fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                 return EXIT_FAILURE;
              }

              if (! DynArray_add(oTokens, psToken))
              {
                 fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                 return EXIT_FAILURE;
              }

              iValueIndex = 0;
              acValue[iValueIndex++] = c;
              eState = STATE_IN_SPECIAL;
            }

            else if ( isspace(c) || (c == '\t'))
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;

               eState = STATE_START;
            }

            //else if (isalpha(c) || (c > 32 && c < 127))
            else
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_LINE, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }
               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }

            break;

          case STATE_IN_SPECIAL:
            if ((c == '\n') || (c == '\0'))
            {
               /* Create a SPECIAL token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_SPECIAL, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }
               iValueIndex = 0;

               return EXIT_SUCCESS;
            }

            else if (c == '"')
            {
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_SPECIAL, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;

               eState = STATE_IN_COMM;
            }

            else if (c == '|')
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_SPECIAL, acValue);
               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_LINE;
            }

            else if ( isspace(c) || (c == '\t'))
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_SPECIAL, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               iValueIndex = 0;

               eState = STATE_START;
            }

            else if ( c == '<' || c == '>')
            {

              acValue[iValueIndex] = '\0';
              psToken = makeToken(TOKEN_SPECIAL, acValue);

              if (psToken == NULL)
              {
                 fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                 return EXIT_FAILURE;
              }

              if (! DynArray_add(oTokens, psToken))
              {
                 fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                 return EXIT_FAILURE;
              }
              iValueIndex = 0;
              eState = STATE_IN_SPECIAL;
            }
            //else if (isalpha(c) || (c > 32 && c < 127))
            else
            {
               /* Create a LINE token. */
               acValue[iValueIndex] = '\0';
               psToken = makeToken(TOKEN_SPECIAL, acValue);

               if (psToken == NULL)
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }

               if (! DynArray_add(oTokens, psToken))
               {
                  fprintf(stderr, "%s: Cannot allocate memory\n", filepath);
                  return EXIT_FAILURE;
               }
               iValueIndex = 0;
               acValue[iValueIndex++] = c;
               eState = STATE_IN_WORD;
            }

            break;

          default:
            assert(0);
      }
   }
}

/*--------------------------------------------------------------------*/
int synLine(DynArray_T oTokens, char *filepath)
/* syntatically analyze the set of tokens. */

/* The '|' token should indicate that the immediate token
after the '|' is another command. If there's no following token after '|',
appropriate error message is printed.
There can be multiple pipe operators in a single command.
Return 1 if successful. Otherwise, return 0.
 */

{
  assert(oTokens != NULL);
  int num_sprhs = 0; // num of ">"
  int num_splhs = 0; // num of "<"
  // only A < B | C > D is possible
  int num_pipe = 0;

  struct Token ** array = (struct Token**)malloc(DynArray_getLength(oTokens)*sizeof(struct Token*));
  if (array == NULL)
  {
    fprintf(stderr, "%s: Cannot allocate memory", filepath);
    return EXIT_FAILURE;
  }
  for(int i = 0; i < DynArray_getLength(oTokens); i++)
  {
    array[i] = DynArray_get(oTokens,i);
  }
  /* Search for pipe, speial errors */
  for(int i = 0; i < DynArray_getLength(oTokens); i++)
  {
    /* Erros */
    if(array[i] -> eType == TOKEN_LINE || array[i] -> eType == TOKEN_SPECIAL)
    {

      if(!strcmp(array[i] -> pcValue, "|"))
      {
        num_pipe++;
        if(num_sprhs == 1) // "A > B | ~~" does not work
        {
          fprintf(stderr, "%s: Multiple redirection of standard in/out\n",filepath);
          free(array);
          return EXIT_FAILURE;
        }
      }
      if(!strcmp(array[i] -> pcValue, "<"))
      {
        if(num_pipe)
        {
          fprintf(stderr, "%s: Multiple redirection of standard in/out\n",filepath);
          free(array);
          return EXIT_FAILURE;
        }
        num_splhs++;
        if(num_splhs==2)
        {
          fprintf(stderr, "%s: Multiple redirection of standard in/out\n",filepath);
          free(array);
          return EXIT_FAILURE;
        }
      }
      if(!strcmp(array[i] -> pcValue, ">"))
      {
        num_sprhs++;
        if(num_sprhs==2)
        {
          fprintf(stderr, "%s: Multiple redirection of standard in/out\n",filepath);
          free(array);
          return EXIT_FAILURE;
        }
      }
      if(i == 0) // first is |
      {
        fprintf(stderr, "%s: Missing command name\n",filepath);
        free(array);
        return EXIT_FAILURE;
      }
      if(i == DynArray_getLength(oTokens)-1) // last is |
      {
        fprintf(stderr, "%s: Pipe or redirection destination not specified\n",filepath);
        free(array);
        return EXIT_FAILURE;
      }
      else if(array[i+1] -> eType == TOKEN_LINE || array[i+1] -> eType == TOKEN_SPECIAL) // command should follow after |, no ||
      {
        fprintf(stderr, "%s: Pipe or redirection destination not specified\n",filepath);
        free(array);
        return EXIT_FAILURE;
      }
    }
  }
  free(array);
  return EXIT_SUCCESS;
}


/*--------------------------------------------------------------------*/

int test(char *filepath)
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
         //exit(EXIT_FAILURE);
      }

      iSuccessful = lexLine(acLine, oTokens, filepath);
      if(!iSuccessful)
      {
          printf("Words: ");
          DynArray_map(oTokens, printWordToken, NULL);
          printf("\n");

          printf("Line: ");
          DynArray_map(oTokens, printLineToken, NULL);
          printf("\n");

          printf("Special: ");
          DynArray_map(oTokens, printSpecialToken, NULL);
          printf("\n");

          printf("Result: \n");
          DynArray_map(oTokens, printAllToken, NULL);
          printf("\n");

      }
      printf("------------------------------------\n");

      synLine(oTokens, filepath);
      printf("\n");

      printf("------------------------------------\n");

      DynArray_map(oTokens, freeToken, NULL);
      DynArray_free(oTokens);
   }

   return 0;
}
