#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
	pid_t iPid;
	iPid = getpid();

	printf("hello process (%d)\n", (int)iPid);

	return 0;
}
