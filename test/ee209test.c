#include <stdio.h>
#include <unistd.h>

int main(){
	printf("%s", getenv("HOME"));
	printf("%s", $HOME);
}
