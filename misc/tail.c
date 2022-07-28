#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_NLINES 10

static void do_tail(FILE* f, long n);
int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stdout, "Usage: %s FILE\n", argv[0]);
  }
  FILE* f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    exit(1);
  }
  do_tail(f, DEFAULT_NLINES);
  fclose(f);
  return 0;
}

void do_tail(FILE* f, long n) {
  int l = 0;
  int r = '\n';
  int c;
  while ((c = getc(f)) != EOF) {
    if (c == '\n') ++l;
    r = c;
  }
  if (r != '\n') ++l;
  rewind(f);
  while ((c = getc(f)) != EOF) {
    if (l <= n) putc(c, stdout);
    if (c == '\n') --l;
  }
}
