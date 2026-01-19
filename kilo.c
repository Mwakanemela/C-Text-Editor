#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<sys/ioctl.h> // ioctl - i/o control
#include<string.h>

// defines
#define CTRL_KEY(k) ((k) & 0x1f)
// data
struct editorConfig {
	int screenrows;
	int screencols;
	struct termios original_terminal_settings;
};

struct editorConfig E;

// append buffer - for creating our own dynamic string data type
struct abuf {
	char *b;
	int len;
};

// an empty buffer - acts as a constructor
#define ABUF_INIT {NULL, 0}

//functions
void enableRawMode();
void disableRawMode();
void die();
void editorProcessKeyPress();
char editorReadKey();
void editorRefreshScreen();
void editorDrawRows(struct abuf *ab);
int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

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
	struct abuf ab = ABUF_INIT;
	
	//hide cursor
	abAppend(&ab, "\x1b[?25l", 6);
	
	//\x1b = 27 = escape character
	//abAppend(&ab, "\x1b[2J", 4);
	
	//reposition the cursor at the top-left corner
	abAppend(&ab, "\x1b[H", 3);
	
	editorDrawRows(&ab);
	abAppend(&ab, "\x1b[H", 3);
	
	//show cursor
	abAppend(&ab, "\x1b[?25h", 6);
	
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
void editorDrawRows(struct abuf *ab) {
	int y;
	for(y = 0; y < E.screenrows; y++) {
		//printf("%d\r\n", y+1); //for displaying number of lines
		abAppend(ab, "~", 1);
		abAppend(ab, "\x1b[K", 3);
		if(y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
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

void abAppend(struct abuf *ab, const char *s, int len) {
	char *new = realloc(ab->b, ab->len + len);
	
	if(new == NULL) return;
	memcpy(&new[ab->len], s, len);
	ab->b = new;
	ab->len += len;
}

void abFree(struct abuf *ab) {
	free(ab->b);
}

// terminal
int getWindowSize(int *rows, int *cols) {
	struct winsize ws;
	
	if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			return -1;
		return getCursorPosition(rows, cols);
	}else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0;
	}
}
int getCursorPosition(int *rows, int *cols) {

	char buf[32];
	unsigned int i = 0;
	
	if(write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
	
	while(i < sizeof(buf) - 1) {
		if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
		if(buf[i] == 'R') break;
		i++;
	}
	buf[i] = '\0';
	
	if(buf[0] != '\x1b' || buf[1] != '[') return -1;
	if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
	
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