#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s PATH\n", argv[1]);
    exit(1);
  }
  size_t len = strlen(argv[1]);
  char buf[1024] = {};
  for (int i = 0; i < len; ++i) {
    buf[i] = argv[1][i];
    if (buf[i] == '/' || i == len - 1) {
      if (mkdir(buf, 0777) == -1) {
        perror(buf);
        exit(1);
      }
    }
  }
}
