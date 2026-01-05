#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>


struct termios original_terminal_settings;

void enableRawMode();
void disableRawMode();

int main() {

	enableRawMode();

	char c;
	
	//read 1 byte from stdin till no bytes to read
	while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
		if(iscntrl(c)) {
			printf("%d\n",c);
		}else{
			printf("%d ('%c')\n", c ,c);
		}
	}
	return 0;
}

void disableRawMode() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_settings);

}

void enableRawMode() {

	tcgetattr(STDIN_FILENO, &original_terminal_settings);
	atexit(disableRawMode);

	struct termios raw = original_terminal_settings;
	raw.c_lflag &= ~(ECHO | ICANON); 	
	// read terminal attributes(keypress)
	//tcgetattr(STDIN_FILENO, &raw);
	
	// modify terminal attributes
	//raw.c_lflag &= ~(ECHO);
	
	// apply terminal attributes
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}