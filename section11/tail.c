#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_NLINES 10
int main(int argc, char* argv[]) {
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [NLINES]\n", argv[0]);
    exit(1);
  }

  int nlines = argc == 2 ? atoi(argv[1]) : DEFAULT_NLINES;
  if (nlines == 0) exit(0);
  if (nlines < 0) {
    fprintf(stderr, "Invalid argument\n");
    exit(1);
  }

  char buf[nlines][1024];
  long wr = 0, wc = 0;
  int wrapped = 0;
  int prev = '\n';
  int c;
  while ((c = fgetc(stdin)) != EOF) {
    buf[wr][wc] = c;
    ++wc;
    if (wc == 1024) {
      fprintf(stderr, "over flow line buffer\n");
      exit(1);
    }
    if (c == '\n') {
      buf[wr][wc] = '\0';
      wc = 0;
      ++wr;
      if (wr == nlines) {
        wr = 0;
        wrapped = 1;
      }
    }
    prev = c;
  }
  if (prev != '\n') {
    buf[wr][wc] = '\n';
    ++wc;
    buf[wr][wc] = '\0';
    ++wc;
  }

  int end = wc != 0 ? wr : (wr + nlines - 1) % nlines;
  int start = wrapped ? (end + 1) % nlines : 0;
  for (int i = start;; i = (i + 1) % nlines) {
    fputs(buf[i], stdout);
    if (i == end) break;
  }
}
