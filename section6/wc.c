#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("specify file(s)");
    exit(1);
  }
  long line = 0;
  for (int i = 1; i < argc; ++i) {
    FILE* f = fopen(argv[i], "r");
    if (!f) {
      perror("argv[i]");
      exit(1);
    }
    int p = '\n';
    int c;
    while ((c = fgetc(f)) != EOF) {
      if (c == (int)'\n') ++line;
      p = c;
    }
    if (p != '\n') ++line;
  }
  printf("%ld\n", line);
}
