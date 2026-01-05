#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>

// defines
#define CTRL_KEY(k) ((k) & 0x1f)
// data
struct termios original_terminal_settings;

//functions
void enableRawMode();
void disableRawMode();
void die();
void editorProcessKeyPress();
char editorReadKey();
int main() {

	enableRawMode();
	
	//read 1 byte from stdin till no bytes to read
	while(1) {
		editorProcessKeyPress();
	}
	return 0;
}

// use-case: 
//	wait for 1 key press and return it 
char editorReadKey() {

	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if(nread == -1 && errno != EAGAIN) die("read");
	}
	return c;
}
void editorProcessKeyPress() {
	char c = editorReadKey();
	
	switch(c) {
		case CTRL_KEY('q'):
			exit(0);
			break;
	}

}
// terminal 
void disableRawMode() {
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_settings) == -1) {
		die("tscattr");
	}
}

void enableRawMode() {

	if(tcgetattr(STDIN_FILENO, &original_terminal_settings) == -1) die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = original_terminal_settings;
	raw.c_lflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_lflag &= ~(OPOST);
	raw.c_lflag &= ~(CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); 
	
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;	
	// read terminal attributes(keypress)
	//tcgetattr(STDIN_FILENO, &raw);
	
	// modify terminal attributes
	//raw.c_lflag &= ~(ECHO);
	
	// apply terminal attributes
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcgetattr");
}

void die(const char *s) {

	perror(s);
	exit(1);
}