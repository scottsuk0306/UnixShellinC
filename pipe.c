/*--------------------------------------------------------------------*/
/* testdupforkexec.c                                                  */
/* Slightly Updated By: Asim                                          */
/* Original Author: Bob Dondero                                       */
/* The dup, fork, and exec system calls.                              */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

enum {PERMISSIONS = 0600};

int main(int argc, char *argv[])
{
   pid_t iPid;

   printf("Parent process (%d)\n", (int)getpid());

   fflush(NULL);
   iPid = fork();
   if (iPid == -1) {perror(argv[0]); exit(EXIT_FAILURE); }

   if (iPid == 0)
   {
      char *apcArgv[2];
      int iFd;
      int iRet;

      iFd = creat("tempfile", PERMISSIONS);
      if (iFd == -1) {perror(argv[0]); exit(EXIT_FAILURE); }

      iRet = close(1);
      if (iRet == -1) {perror(argv[0]); exit(EXIT_FAILURE); }

      iRet = dup(iFd);
      if (iRet == -1) {perror(argv[0]); exit(EXIT_FAILURE); }

      iRet = close(iFd);
      if (iRet == -1) {perror(argv[0]); exit(EXIT_FAILURE); }

      apcArgv[0] = "date";
      apcArgv[1] = NULL;
      execvp(apcArgv[0], apcArgv);
      perror(argv[0]);
      exit(EXIT_FAILURE);
   }

   iPid = wait(NULL);
   if (iPid == -1) {perror(argv[0]); exit(EXIT_FAILURE); }

   /* This code is executed by only the parent process. */
   printf("Parent process (%d)\n", (int)getpid());

   return 0;
}

/*--------------------------------------------------------------------*/

/* Sample execution:

$ gcc209 testdupforkexec.c -o testdupforkexec

$ testdupforkexec
Parent process (16581)
Parent process (16581)

$ cat tempfile
Tue Dec 12 10:22:14 EDT 2011

*/
