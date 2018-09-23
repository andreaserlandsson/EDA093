#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main() {
	int i;
	char *str[] = {"|", "/", "-", "\\", "|", "/", "-","\\", "|", '\0'};

	for(i = 0; str[i] != '\0'; i++) {
		printf("%s\r", str[i]);
		fflush(stdout);
		sleep(1);	
	}
}

