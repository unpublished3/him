#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

void enableRawMode() {
  struct termios raw;

  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode() {
  struct termios raw;

  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= (ECHO);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  char c;

  enableRawMode();
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
    ;
  disableRawMode();
  return 0;
}
