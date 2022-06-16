#include <stdio.h>
#include <stdlib.h>

static void do_head(FILE* f, long l);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s n\n", argv[0]);
    exit(1);
  }

  if (argc == 2) {
    do_head(stdin, atol(argv[1]));
  } else {
    for (int i = 2; i < argc; ++i) {
      FILE* f = fopen(argv[i], "r");
      if (!f) {
        perror(argv[i]);
        exit(1);
      }
      do_head(f, atol(argv[1]));
      if (fclose(f) == EOF) {
        perror(argv[i]);
        exit(1);
      }
    }
  }
}

void do_head(FILE* f, long l) {
  int c;
  while (l > 0 && (c = fgetc(f)) != EOF) {
    if (fputc(c, stdout) == EOF) exit(1);
    if (c == '\n') --l;
  }
}
