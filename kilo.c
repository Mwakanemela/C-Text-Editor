#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<sys/ioctl.h> // ioctl - i/o control

// defines
#define CTRL_KEY(k) ((k) & 0x1f)
// data
struct editorConfig {
	int screenrows;
	int screencols;
	struct termios original_terminal_settings;
};

struct editorConfig E;

//functions
void enableRawMode();
void disableRawMode();
void die();
void editorProcessKeyPress();
char editorReadKey();
void editorRefreshScreen();
void editorDrawRows();
int getWindowSize(int *rows, int *cols);

// initialize - init
void initializeEditor() {
	if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("Fialed to getWindowSize");
}

int main() {

	enableRawMode();
	initializeEditor();	
	//read 1 byte from stdin till no bytes to read
	while(1) {
		editorRefreshScreen();
		editorProcessKeyPress();
	}
	return 0;
}

//output
void editorRefreshScreen() {
	//\x1b = 27 = escape character
	write(STDOUT_FILENO, "\x1b[2J", 4);
	
	//reposition the cursor at the top-left corner
	write(STDOUT_FILENO, "\x1b[H", 3);
	
	editorDrawRows();
	write(STDOUT_FILENO, "\x1b[H", 3);
}
void editorDrawRows() {
	int y;
	for(y = 0; y < E.screenrows; y++) {
		//printf("%d\r\n", y+1); //for displaying number of lines
		write(STDOUT_FILENO, "~\r\n", 3);
	}
}
// input
void editorProcessKeyPress() {
	char c = editorReadKey();
	
	switch(c) {
		case CTRL_KEY('q'):
			//\x1b = 27 = escape character
			write(STDOUT_FILENO, "\x1b[2J", 4);
			
			//reposition the cursor at the top-left corner
			write(STDOUT_FILENO, "\x1b[H", 3);
			
			exit(0);
			break;
	}

}
// terminal
int getWindowSize(int *rows, int *cols) {
	struct winsize ws;
	
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		return -1;
	}else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
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
 
void disableRawMode() {
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_terminal_settings) == -1) {
		die("tscattr");
	}
}

void enableRawMode() {

	if(tcgetattr(STDIN_FILENO, &E.original_terminal_settings) == -1) die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = E.original_terminal_settings;
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

	//\x1b = 27 = escape character
	write(STDOUT_FILENO, "\x1b[2J", 4);
	
	//reposition the cursor at the top-left corner
	write(STDOUT_FILENO, "\x1b[H", 3);
	
	perror(s);
	exit(1);
}