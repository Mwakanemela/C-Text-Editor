// macros to make my editor portable
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdio.h>
#include<errno.h>
#include<sys/ioctl.h> // ioctl - i/o control
#include<string.h>
#include<sys/types.h>


// defines
#define MWAKA_EDITOR_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey {
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	DEL_KEY,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN
};

// data

// a data type for storing a row of text in my editor
typedef struct erow {
	int size;
	char *chars;
}erow;

struct editorConfig {
	int cx, cy;
	int rowoff;
	int screenrows;
	int screencols;
	int numrows;
	erow *row;
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
int editorReadKey();
void editorRefreshScreen();
void editorDrawRows(struct abuf *ab);
int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);
void editorMoveCursor(int key);
void editorOpen(char *filename);
void editorAppendRow(char *s, size_t len);
void editorScroll();

// initialize - init
void initializeEditor() {
	E.cx = 0;
	E.cy = 0;
	E.rowoff = 0;
	E.numrows = 0;
	E.row = NULL;
	
	if(getWindowSize(&E.screenrows, &E.screencols) == -1) die("Fialed to getWindowSize");
}

int main(int argc, char *argv[]) {

	enableRawMode();
	initializeEditor();
	
	if(argc >= 2) {
		editorOpen(argv[1]);
	}
	
		
	//read 1 byte from stdin till no bytes to read
	while(1) {
		editorRefreshScreen();
		editorProcessKeyPress();
	}
	return 0;
}

// FILE i/o
//function for opening and reading a file from disk
void editorOpen(char *filename) {
	FILE *fp = fopen(filename, "r");
	if(!fp) die("fopen");
	
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	while((linelen = getline(&line, &linecap, fp)) != -1) {
		while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
		editorAppendRow(line, linelen);
	}
	free(line);
	fclose(fp);	
}

//row operations
void editorAppendRow(char *s, size_t len) {
	E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
	
	int at = E.numrows;
	
	E.row[at].size = len;
	E.row[at].chars = malloc(len + 1);
	memcpy(E.row[at].chars, s, len);
	E.row[at].chars[len] = '\0';
	E.numrows++;	
}

//output
void editorScroll() {
	//check if the cursor is above the visible window
	if(E.cy < E.rowoff) {
		//scroll up to where the cursor is
		E.rowoff = E.cy;
	}
	
	//check if the cursor is past the bottom of the visible window
	if(E.cy >= E.rowoff + E.screenrows) {
		E.rowoff = E.cy - E.screenrows + 1;
	}
}

void editorRefreshScreen() {
	editorScroll();
	struct abuf ab = ABUF_INIT;
	
	//hide cursor
	abAppend(&ab, "\x1b[?25l", 6);
	
	//\x1b = 27 = escape character
	//abAppend(&ab, "\x1b[2J", 4);
	
	//reposition the cursor at the top-left corner
	abAppend(&ab, "\x1b[H", 3);
	
	editorDrawRows(&ab);
	
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, E.cx + 1);
	abAppend(&ab, buf, strlen(buf));
	
	//show cursor
	abAppend(&ab, "\x1b[?25h", 6);
	
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}
void editorDrawRows(struct abuf *ab) {
	int y;
	for(y = 0; y < E.screenrows; y++) {
		int filerow = y + E.rowoff;
		if(filerow >= E.numrows) {
			//display welcome msg wen user starts program with no args
			if(E.numrows == 0 && y == E.screenrows / 3) {
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome), "Mwaka editor -- version %s", MWAKA_EDITOR_VERSION);
				if(welcomelen > E.screencols) welcomelen = E.screencols;
				//centering welcome message
				int padding = (E.screencols - welcomelen) / 2;
				if(padding) {
					abAppend(ab, "~", 1);
					padding--;
				}
				while(padding--) abAppend(ab, " ", 1);
				abAppend(ab, welcome, welcomelen);
			}else {
				//printf("%d\r\n", y+1); //for displaying number of lines
				abAppend(ab, "~", 1);
			}
		}else {
			int len = E.row[filerow].size;
			if(len > E.screencols) len = E.screencols;
			abAppend(ab, E.row[filerow].chars, len);
		}
		abAppend(ab, "\x1b[K", 3);
		if(y < E.screenrows - 1) {
			abAppend(ab, "\r\n", 2);
		}
	}
}
// input
void editorProcessKeyPress() {
	int c = editorReadKey();
	
	switch(c) {
		case CTRL_KEY('q'):
			//\x1b = 27 = escape character
			write(STDOUT_FILENO, "\x1b[2J", 4);
			
			//reposition the cursor at the top-left corner
			write(STDOUT_FILENO, "\x1b[H", 3);
			
			exit(0);
			break;
		case HOME_KEY:
			E.cx = 0;
			break;
		case END_KEY:
			E.cx = E.screencols - 1;
			break;
		case PAGE_UP:
		case PAGE_DOWN:
			{
				int times = E.screenrows;
				while(times--)
					editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
			break;
		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			editorMoveCursor(c);
			break;
	}

}

void editorMoveCursor(int key) {
	switch(key) {
		case ARROW_LEFT:
			if(E.cx != 0) {
				E.cx--;
			}
			break;
		case ARROW_RIGHT: 
			if(E.cx != E.screencols -1) {
				E.cx++;
			}
			break;
		case ARROW_UP:
			if(E.cy != 0) {
				E.cy--;
			}
			break;
		case ARROW_DOWN:
			if(E.cy < E.numrows) {
				E.cy++;
			}
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
int editorReadKey() {

	int nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if(nread == -1 && errno != EAGAIN) die("read");
	}
	if(c == '\x1b') {
		char seq[3];
		// if esc is pressed
		if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
		
		// if arrow keys are pressed
		if(seq[0] == '[') {
			if(seq[1] >= '0' && seq[1] <= '9') {
				if(read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if(seq[2] == '~') {
					switch(seq[1]) {
						case '1': return HOME_KEY;
						case '3': return DEL_KEY;
						case '4': return END_KEY;
						case '5': return PAGE_UP;
						case '6': return PAGE_DOWN;
						case '7': return HOME_KEY;
						case '8': return END_KEY;
					}
				}
			}else {
				switch (seq[1]) {
					case 'A': return ARROW_UP;
					case 'B': return ARROW_DOWN;
					case 'C': return ARROW_RIGHT;
					case 'D': return ARROW_LEFT;
					case 'H': return HOME_KEY;
					case 'F': return END_KEY;
				}
			}
		}else if(seq[0] == 'O') {
			switch(seq[1]) {
				case 'H': return HOME_KEY;
				case 'F': return END_KEY;
			}
		}
		return '\x1b';
		
	}else {
		return c;
	}
	
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