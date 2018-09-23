#include <stdio.h>
#include <unistd.h>

int main() {
	char cwd[255];
	printf("%s\n", getlogin());
	printf("%s\n", get_current_dir_name());
}

