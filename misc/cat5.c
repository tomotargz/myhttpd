#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void die(const char* s) {
  perror(s);
  exit(1);
}

int visualize;

int main(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "v")) != -1) {
    switch (opt) {
      case 'v':
        visualize = 1;
        break;
      case '?':
        fprintf(stderr, "Usage: %s [-v] [FILE ...]\n", argv[0]);
        exit(1);
    }
  }

  if (optind == argc) {
    printf("file not given\n");
    exit(1);
  }

  for (int i = optind; i < argc; ++i) {
    FILE* f = fopen(argv[i], "r");
    if (!f) die(argv[i]);
    int c;
    while ((c = getc(f)) != EOF) {
      if (visualize && c == '\t') {
        putc('\\', stdout);
        putc('t', stdout);
        continue;
      }
      if (visualize && c == '\n') {
        putc('$', stdout);
        putc('\n', stdout);
        continue;
      }
      putc(c, stdout);
    }
    if (fclose(f) == EOF) die(argv[i]);
  }
}
