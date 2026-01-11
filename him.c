/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_ssize_t.h>
#include <sys/ioctl.h>
#include <sys/ttycom.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig {
  struct termios orig_termios;
  int screenRows;
  int screenCols;
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die("tcgetattr");
  atexit(disableRawMode);
  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 100;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    if (nread == -1 && errno != EAGAIN)
      die("read");

  return c;
}

int getWidowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  }
  *rows = ws.ws_row;
  *cols = ws.ws_row;
  return 0;
}

/*** input ***/

void processEditorKeypress() {
  char c = editorReadKey();

  switch (c) {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;
  }
}

/*** output ***/
void editorClearScreen() { write(STDOUT_FILENO, "\x1b[2J", 4); }

void editorRepositionCursor() { write(STDOUT_FILENO, "\x1b[;H", 4); }

void editorDrawRows() {
  int y;
  char printChar[16];
  for (y = 0; y < E.screenRows; y++) {
    sprintf(printChar, "%d\r\n", y);
    write(STDOUT_FILENO, printChar, strlen(printChar));
  }
}

/*** init ***/
void initEditor() {
  if (getWidowSize(&E.screenRows, &E.screenRows) == -1)
    die("getWindowSize");
}

int main() {
  enableRawMode();
  initEditor();

  while (1) {
    editorRepositionCursor();
    editorClearScreen();
    editorDrawRows();
    processEditorKeypress();
  }

  return 0;
}
