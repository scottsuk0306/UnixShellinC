#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// argc: 변수의 개수
// argv: ls -al 
int main(int argc, char *argv[]){
    char **new_argv;
    // char command[] = "ls";
    int idx;

    new_argv = (char **)malloc(sizeof(char *)*(argc+1));

    // new_argv[0] = command;


    for (idx = 0; idx < argc; idx++){
        new_argv[idx] = argv[idx];
    }
    
    new_argv[argc] = NULL;
    
	for (idx = 0; idx < argc; idx++){
        fprintf(stderr, "%s ",new_argv[idx]);
    }
    fprintf(stderr, "\n");

    if(execvp("mkdir",new_argv) == -1){
        fprintf(stderr, "error: %s\n",strerror(errno));
        return 1;
    }


    printf("this part should not be executed");

    return 0;
}
