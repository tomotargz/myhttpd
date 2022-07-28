#include <stdio.h>
#include <stdlib.h>

static void do_cat(FILE* f);
static void putc_or_die(int c);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    do_cat(stdin);
  } else {
    for (int i = 1; i < argc; ++i) {
      FILE* f = fopen(argv[i], "r");
      if (!f) {
        perror(argv[i]);
        exit(1);
      }
      do_cat(f);
      fclose(f);
    }
  }
  exit(0);
}

void do_cat(FILE* f) {
  int c;
  while ((c = fgetc(f)) != EOF) {
    if (c == (int)'\t') {
      putc_or_die((int)'\\');
      putc_or_die((int)'t');
    } else if (c == (int)'\n') {
      putc_or_die((int)'$');
      putc_or_die((int)'\n');
    } else {
      putc_or_die(c);
    }
  }
}

void putc_or_die(int c) {
  if (putchar(c) == EOF) exit(1);
}
