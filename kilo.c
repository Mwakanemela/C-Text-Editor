#include<unistd.h>

int main() {
	char c;
	
	//read 1 byte from stdin till no bytes to read
	while(read(STDIN_FILENO, &c, 1) == 1);
	return 0;
}