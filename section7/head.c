#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void do_head(FILE* f, long l);

static const struct option longopts[] = {
    {"lines", required_argument, NULL, 'n'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0},
};

#define DEFAULT_NLINES 10

int main(int argc, char* argv[]) {
  int opt;
  long nlines = DEFAULT_NLINES;
  while ((opt = getopt_long(argc, argv, "n:", longopts, NULL)) != -1) {
    switch (opt) {
      case 'n':
        nlines = atol(optarg);
        break;
      case 'h':
        printf("Usage: %s [-n LINES] [FILE ...]\n", argv[0]);
        exit(0);
      case '?':
        fprintf(stderr, "Usage: %s [-n LINES] [FILE ...]\n", argv[0]);
        exit(1);
    }
  }

  if (optind == argc) {
    do_head(stdin, nlines);
  } else {
    for (int i = optind; i < argc; ++i) {
      FILE* f = fopen(argv[i], "r");
      if (!f) {
        perror(argv[i]);
        exit(1);
      }
      do_head(f, nlines);
      fclose(f);
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
