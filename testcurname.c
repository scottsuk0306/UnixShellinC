#include <stdio.h>
#include <sys/auxv.h>

int main(int argc, char **argv)
{
    printf("%s\n", (char *)getauxval(AT_EXECFN));
    return(0);
}
