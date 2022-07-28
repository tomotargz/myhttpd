#include <stdio.h>
#include <stdlib.h>

static void die(const char* s) {
  perror(s);
  exit(1);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("file not given\n");
    exit(1);
  }
  for (int i = 1; i < argc; ++i) {
    FILE* f = fopen(argv[i], "r");
    if (!f) die(argv[i]);
    char buf[1024];
    while (!feof(f)) {
      size_t r = fread(buf, 1, sizeof buf, f);
      if (ferror(f)) die(argv[i]);
      size_t w = fwrite(buf, 1, r, stdout);
      if (w != r) die(argv[i]);
    }
    if (fclose(f) == EOF) die(argv[i]);
  }
}
