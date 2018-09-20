#include <stdio.h>
#include <unistd.h>

int main(void) {

	char cmd[] = "ls";
	char *argv[3];
	
	argv[0] = "ls";
	argv[1] = "-la";
	argv[2] = '\0';

	execvp(cmd, argv); //This will run "ls -la" as if it were a command

	return 0;
}
