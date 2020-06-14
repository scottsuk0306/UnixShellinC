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
#include <signal.h>

/**
 * Executes the command "cat scores | grep Villanova | cut -b 1-10".
 * This quick-and-dirty version does no error checking.
 *
 * @author Jim Glenn
 * @version 0.1 10/4/2004
 */

int main(int argc, char **argv)
{
  int status;
  int i;
  int numPipes;

  // make multiple pipes for each "|"
  int pipesfds[2*numPipes];

  /* parent creates all needed pipes at the start */
  for( i = 0; i < numPipes; i++ ){
      if( pipe(pipefds + i*2) < 0 ){
          perror("Pipe Error"); // not sure about this
          exit(EXIT_FAILURE);
      }
  }

  char *cmd[numPipes + 1]; // if 2 pipe, 3 commands exist
  // splice acLine
  // for example, if A | B | C, then cmd[3] = {A, B, C}
  char *command[numPipes + 1];
  // first token for each cmd (line)
  // for example, if ls | grep .txt, then command[2] = ls, grep

  int cmdIndex = 0
  while(cmd[cmd_Index])
  {
      pid = fork()
      if(pid == 0){
          /* child gets input from the previous command,
              if it's not the first command */
          if(cmdIndex != 0){
              if( dup2(pipefds[(cmdIndex-1)*2], 0) < )
              {
                perror("Pipe Error"); // not sure about this
                exit(EXIT_FAILURE);
              }
          }
          /* child outputs to next command, if it's not
              the last command */
          if(cmdIndex != numPipes){
              if( dup2(pipefds[cmdIndex*2+1], 1) < 0 )
              {
                perror("Pipe Error"); // not sure about this
                exit(EXIT_FAILURE);
              }
          }
          for(i = 0; i < 2*numPipes; i++)
          {
              close(pipefds[i]);
          }
          execvp(command, cmd[cmdIndex]);
          // cat, ls, echo, and other commands
          // setenv unsetenv cd exit has to be realized
          fprintf(stderr, "%s: %s\n",command,strerror(errno));
          exit(EXIT_FAILURE);
      } else if( pid < 0 ){
          perror(argv[0]); // should be changed to filepath
          exit(EXIT_FAILURE);
      }
      cmdIndex++
  }

  /* parent closes all of its copies at the end */
  for( i = 0; i < 2 * num-pipes; i++ )
  {
      close( pipefds[i] );
  }
}
