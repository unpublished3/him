/*** includes ***/

#include <_string.h>
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
#define ABUF_INIT {NULL, 0}

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

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

int getWidowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    return getCursorPosition(rows, cols);
  }
  *rows = ws.ws_row;
  *cols = ws.ws_col;
  return 0;
}

/*** append buffer ***/

struct abuf {
  char *b;
  int len;
};

void abAppend(struct abuf *ab, const char *c, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL)
    return;
  memcpy(&new[ab->len], c, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) { free(ab->b); }

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

void editorDrawRows(struct abuf *ab) {
  int y;
  char printChar[16];
  for (y = 1; y <= E.screenRows; y++) {
    sprintf(printChar, "%d", y);
    abAppend(ab, printChar, strlen(printChar));
    if (y != E.screenRows)
      abAppend(ab, "\r\n", 2);
  }
}

void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;
  abAppend(&ab, "\x1b[2J", 4);
  abAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);

  abAppend(&ab, "\x1b[H", 3);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}
/*** init ***/
void initEditor() {
  if (getWidowSize(&E.screenRows, &E.screenCols) == -1)
    die("getWindowSize");
}

int main() {
  enableRawMode();
  initEditor();

  while (1) {
    editorRefreshScreen();
    processEditorKeypress();
  }

  return 0;
}
